#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct
{
    // Should be "IWAD" or "PWAD".
    char                identification[4];
    int                 numlumps;
    int                 infotableofs;

} wadinfo_t;

int main()
{
	printf("This is fun!\r\n");

	FILE *fp = fopen("sd:test.jpg", "rb");
	if (fp)
	{
		printf("File's there, doing seek/read check.\r\n");

		// Grab a tiny memory chunk
		uint8_t *buffer = new uint8_t[8];

		// Read 16 byte blocks
		for (uint32_t i=0;i<5;++i)
		{
			fread(buffer, 8, 1, fp);
			for (uint32_t j=0;j<8;++j)
				printf("%.2X ", buffer[j]);
			printf("\r\n");
		}
		// Read non-adjacent blocks
		for (uint32_t i=0;i<5;++i)
		{
			fseek(fp, i*32, SEEK_SET);
			fread(buffer, 8, 1, fp);
			for (uint32_t j=0;j<8;++j)
				printf("%.2X ", buffer[j]);
			printf("\r\n");
		}

		delete [] buffer;

		fclose(fp);

		printf("Now the same with read() instead of fread()\r\n");
		int handle;
		if ( (handle = open ("sd:doom1.wad", O_RDONLY /*| O_BINARY*/)) != -1)
		{
			printf("Open succeeded, reading\r\n");
			wadinfo_t header;
        	read (handle, &header, sizeof(header));
			printf("header: %c%c%c%c\r\n",
			header.identification[0],
			header.identification[1],
			header.identification[2],
			header.identification[3] );
			close (handle);
		}
		else
			printf("oops, can't open sd:doom1.wad\r\n");
	}
	else
		printf("File's not there.\r\n");

	return 0;
}
