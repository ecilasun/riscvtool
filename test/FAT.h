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

// the BIOS Parameter Block (in Volume Boot Record)
// See: http://www.ntfs.com/fat-partition-sector.htm
typedef struct {
    char            jmp[3]; // Jump instruction (EB 3C 90 for x86)
    char            oem[8]; // Usually 'MSDOS5.0'
    unsigned short  bps;    // Bytes per sector (should read 512, 1024, 2048, 4096)
    unsigned char   spc;    // Sectors per cluster (1,2,4,8,16,32,64,128)
    unsigned short  rsc;    // Number of reserved sectors (1 for FAT16, 32 for FAT32)
    unsigned char   nf;     // File allocation table copies (usually 2)
    unsigned short  nr;     // Root directory entry count (225 for FAT16, 0 for FAT32)
    unsigned short  ts16;   // Num sectors (invalid for FAT32)
    unsigned char   media;  // Media type (0xF8 == hdd)
    unsigned short  spf16;  // Sectors per file allocation table (0 for FAT32)
    unsigned short  spt;    // Sectors per track
    unsigned short  nh;     // Number of heads
    unsigned int    hs;     // Hidden sectors
    unsigned int    ts32;   // Total sectors in filesystem
    unsigned int    spf32;  // Sectors per FAT (FAT32)
    unsigned int    flg;    // Mirror flags
    unsigned int    rc;
    char            vol[6];
    char            fst[8];
    char            dmy[20];
    char            fst2[8];
    // Bytes 510/511 read '0x55AA'
    char            padding[512]; // Provide enough space so we can read a single block onto this buffer without overrun
} __attribute__((packed)) bpb_t;

// directory entry structure
typedef struct {
    char            name[8];
    char            ext[3];
    char            attr[9];
    unsigned short  ch;
    unsigned int    attr2;
    unsigned short  cl;
    unsigned int    size;
} __attribute__((packed)) fatdir_t;

struct FATCONTEXT
{
    __attribute__((aligned(4))) bpb_t bpb;              // BIOS parameter block
    __attribute__((aligned(4))) unsigned char mbr[512]; // Master boot record
    __attribute__((aligned(4))) fatdir_t dir[512];      // Root directory entries (should be more, this is only enough for FAT16)
    __attribute__((aligned(4))) unsigned int FAT[2048];  // This can be quite large (3x512 for now)
};

struct FILEHANDLE
{
    unsigned int file_exists;
    unsigned int file_first_cluster_backup;
    unsigned int file_first_cluster;
    unsigned int file_size;
    unsigned int first_access;
    unsigned int file_offset;
    unsigned int file_open;
    unsigned int data_sec;
    unsigned int *fat32;
    unsigned short *fat16;
    bpb_t *bpb;
    struct FATCONTEXT *ctx;
};

int fat_getpartition();
int fat_getfilesize(struct FILEHANDLE *fh);
int fat_find_file(const char *fn, struct FILEHANDLE *fh);
int fat_list_files(char *target);
int fat_readfile(struct FILEHANDLE *fh, char *buffer, int read_bytes, int *total_read);
int fat_openfile(const char *fn, struct FILEHANDLE *fh);
void fat_closefile(struct FILEHANDLE *fh);

