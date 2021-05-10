/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "FAT.h"
#include "SDCARD.h"
#include "utils.h"
#include <string.h>

#define EAlignUp(_x_, _align_) ((_x_ + (_align_ - 1)) & (~(_align_ - 1)))

unsigned int partitionlba;
struct FATCONTEXT fatcontext;
//extern unsigned char _end;

/**
 * Get the starting LBA address of the first partition
 * so that we know where our FAT file system starts, and
 * read that volume's BIOS Parameter Block
 */
int fat_getpartition()
{
    unsigned char *mbr = fatcontext.mbr;
    bpb_t *bpb = &fatcontext.bpb;
    // Read the partition table
    if(SDReadMultipleBlocks(mbr,1,0) != -1)
    {
        //printf(" block0 read\n");
        // check magic
        if(mbr[510]!=0x55 || mbr[511]!=0xAA)
        {
            //printf("ERROR: Bad magic in MBR\n");
            return 0;
        }

        // check partition type
        if(mbr[0x1C2]!=0xE/*FAT16 LBA*/ && mbr[0x1C2]!=0xC/*FAT32 LBA*/)
        {
            //printf("ERROR: Wrong partition type\n");
            return 0;
        }
        //printf(" type OK\n");

        partitionlba = *((unsigned int*)((unsigned long)mbr + 0x1C6));

        // read the boot record
        //printf(" reading boot record at LBA %d\n", partitionlba);
        if(SDReadMultipleBlocks((unsigned char*)bpb, 1, partitionlba) == -1)
        {
            //printf("ERROR: Unable to read boot record\n");
            return 0;
        }
        //printf(" partition boot record read\n");
        // check file system type. We don't use cluster numbers for that, but magic bytes
        if( !(bpb->fst[0]=='F' && bpb->fst[1]=='A' && bpb->fst[2]=='T') &&
            !(bpb->fst2[0]=='F' && bpb->fst2[1]=='A' && bpb->fst2[2]=='T'))
        {
            //printf("ERROR: Unknown file system type %c%c%c\n", bpb->fst[0], bpb->fst[1], bpb->fst[2]);
            return 0;
        }

        // We do not load any FAT entry initially
        fatcontext.CachedFATPage = 0xFFFFFFFF;

        //printf(" SUCCESS\n");
        return 1;
    }
    //printf("ERROR: Could not read block\n");
    return 0;
}

/**
 * Find a file in root directory entries
 */
int fat_find_file(const char *fn, struct FILEHANDLE *fh)
{
    fh->file_exists = 0;
    fh->file_first_cluster_backup = 0;
    fh->file_first_cluster = 0;
    fh->file_size = 0;
    fh->first_access = 1;
    fh->file_offset = 0;
    fh->file_open = 0;
    fh->data_sec = 0;
    fh->fat32 = nullptr;
    fh->fat16 = nullptr;

    bpb_t *bpb = &fatcontext.bpb;
    fatdir_t *dir = fatcontext.dir;
    unsigned int root_sec, s;
    // find the root directory's LBA
    root_sec = ((bpb->spf16 ? bpb->spf16 : bpb->spf32)*bpb->nf) + bpb->rsc;
    s = bpb->nr*sizeof(fatdir_t);
    if(bpb->spf16==0)
    {
        // adjust for FAT32
        root_sec += (bpb->rc-2)*bpb->spc;
    }
    // add partition LBA
    root_sec += partitionlba;
    // load the root directory
    if(SDReadMultipleBlocks((unsigned char*)dir, s/512+1, root_sec) != -1)
    {
        // iterate on each entry and check if it's the one we're looking for
        for(; dir->name[0]!=0; dir++)
        {
            // Is it a valid entry?
            if(dir->name[0]==0xE5 || dir->attr[0]==0xF)
                continue;

            // Filename match, extension match
            if(!__builtin_memcmp(dir->name,fn,11))
            {
                fh->file_exists = 1;
                fh->file_size = dir->size;
                fh->file_first_cluster_backup = ((unsigned int)dir->ch)<<16|dir->cl;
                fh->file_first_cluster = fh->file_first_cluster_backup;
                return 1;
            }
        }
    }
    else
    {
        //printf("ERROR: Unable to load root directory\n");
    }

    //printf("ERROR: file not found\n");
    return 0;
}

int fat_list_files(char *target)
{
    bpb_t *bpb = &fatcontext.bpb;

    fatdir_t *dir = fatcontext.dir;
    unsigned int root_sec, s;
    // find the root directory's LBA
    root_sec = ((bpb->spf16 ? bpb->spf16 : bpb->spf32)*bpb->nf) + bpb->rsc;
    s = bpb->nr*sizeof(fatdir_t);
    if(bpb->spf16==0)
    {
        // adjust for FAT32
        root_sec += (bpb->rc-2)*bpb->spc;
    }
    // add partition LBA
    root_sec += partitionlba;

    // load the root directory
    int col = 0;
    if(SDReadMultipleBlocks((unsigned char*)dir, s/512+1, root_sec) != -1)
    {
        // iterate on each entry and check if it's the one we're looking for
        for(; dir->name[0]!=0; dir++)
        {
            // Is it a valid entry?
            if(dir->name[0]==0xE5 || dir->attr[0]==0xF)
                continue;

            if (target==nullptr)
            {
                PrintDMA(0,col,11,dir->name); // name:8 + ext:3
                col+=8;
            }
            else
            {
                memcpy(target, dir->name, 8);
                target[8] = 0;
                //strncat(target, dir->name, 8);
                strcat(target, ".");
                strncat(target, dir->ext, 3);
                strcat(target, "\r\n");
            }
        }
    }
    else
    {
        //printf("ERROR: Unable to load root directory\n");
    }

    //printf("ERROR: file not found\n");
    return 0;
}

int fat_openfile(const char *fn, struct FILEHANDLE *fh)
{
    if (fat_find_file(fn, fh))
    {
        // BIOS Parameter Block
        fh->bpb = &fatcontext.bpb;
        // File allocation tables. We choose between FAT16 and FAT32 dynamically
        fh->fat32 = fatcontext.FAT;//(unsigned int*)(&fh->bpb+fh->bpb->rsc*512); //(unsigned int*)0x00006000;//&_end+0x20000;//(unsigned int*)fatcontext.fat;
        fh->fat16 = (unsigned short*)fh->fat32;
        // find the LBA of the first data sector
        fh->data_sec = ((fh->bpb->spf16 ? fh->bpb->spf16 : fh->bpb->spf32)*fh->bpb->nf) + fh->bpb->rsc;
        if(fh->bpb->spf16>0)
        {
            // adjust for FAT16
            fh->data_sec += (fh->bpb->nr*sizeof(fatdir_t)+511)>>9;
        }
        // add partition LBA
        fh->data_sec += partitionlba;

        //printf("> reading FAT for %s @ %.8X\n", fn, fh->fat32);
        // Load file allocation table into top of heap
        /*unsigned int file_allocation_table_size = (fh->bpb->spf16 ? fh->bpb->spf16 : fh->bpb->spf32) + fh->bpb->rsc;
        if (file_allocation_table_size != 0)
        {
            EchoInt(file_allocation_table_size);
            if (SDReadMultipleBlocks((unsigned char*)fh->fat32, (file_allocation_table_size/512)+1, partitionlba+1) == -1)
            {
                //printf("ERROR: FAT could not read table for %s\n", fn);
                return 0;
            }
        }*/

        /*while(1)
        {
            if (SDReadMultipleBlocks((unsigned char*)fh->fat32, 1, fh->file_first_cluster_backup) == -1)
            cluster = get_next_cluster_id(fs, cluster);
        }*/

        fh->file_open = 1;
    }

    return fh->file_open;
}

void fat_closefile(struct FILEHANDLE *fh)
{
    if (fh->file_open)
    {
        fh->file_exists = 0;
        fh->file_first_cluster_backup = 0;
        fh->file_first_cluster = 0;
        fh->file_size = 0;
        fh->first_access = 1;
        fh->file_offset = 0;
        fh->file_open = 0;
        fh->data_sec = 0;
        fh->fat32 = nullptr;
        fh->fat16 = nullptr;
        //fh->bpb = 0;
    }
}

/**
 * Read a file into memory
 */
int fat_readfile(struct FILEHANDLE *fh, char *buffer, int read_bytes, int *total_read)
{
    *total_read = 0;
    unsigned int cluster = fh->file_first_cluster;

    // Iterate on cluster chain
    unsigned char *target_buffer = (unsigned char *)buffer;
    int file_size = fh->bpb->spc*fh->bpb->bps;
    //printf("> reading %dbytes from %dbytes file\n", read_bytes, file_size);
    while(cluster>1 && cluster<0xFFF8)
    {
        // load all sectors in a cluster
        unsigned int source_block = (cluster-2)*fh->bpb->spc+fh->data_sec;
        if (SDReadMultipleBlocks(target_buffer, fh->bpb->spc, source_block) == -1)
        {
            EchoUART("Error reading file block\r\n");
            return 0;
        }

        // Move pointer, sector per cluster * bytes per sector
        int actual_read = fh->bpb->spc*fh->bpb->bps;
        target_buffer += actual_read;
        fh->file_offset += actual_read;
        *total_read += actual_read;

        // Now the tricky part.
        // We need to re-load the 512 byte FAT table slice
        // where 'cluster' lies within
        uint32_t FAT_page = fh->bpb->spf16 > 0 ? cluster/256 : cluster/128;
        if (FAT_page != fatcontext.CachedFATPage)
        {
            fatcontext.CachedFATPage = FAT_page;
            uint32_t partition_begin_sector = 0;
            uint32_t fat_begin_sector = partition_begin_sector + fh->bpb->rsc;
            unsigned int sector[128];
            if (SDReadMultipleBlocks((unsigned char*)sector, 1, fat_begin_sector + fatcontext.CachedFATPage) != -1)
            {
                for (int i=0;i<128;++i)
                    fatcontext.FAT[i] = sector[i];
            }
            else
            {
                EchoUART("Error reading FAT block\r\n");
                return 0;
            }
        }

        // Get the next cluster in chain
        cluster = fh->bpb->spf16 > 0 ? fh->fat16[cluster%256] : fh->fat32[cluster%128];
        if (*total_read >= read_bytes || fh->file_offset >= (uint32_t)file_size)
            break;
    }

    // NOTE: For subsequent READ calls, this needs to advance forward
    // 'seek' operations to START of file can then restore from file_first_cluster_backup
    fh->file_first_cluster = cluster;

    return 1;
}

int fat_getfilesize(struct FILEHANDLE *fh)
{
    return (int)fh->file_size;
}
