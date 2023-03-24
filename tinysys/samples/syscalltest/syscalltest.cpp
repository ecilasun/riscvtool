#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main()
{
	printf("This is fun!\r\n");

	FILE *fp = fopen("sd:test.jpg", "rb");
	if (fp)
	{
		printf("File's there, reading.\r\n");
		// Grab a tiny memory chunk
		uint8_t *buffer = new uint8_t[8];
		// Read 16 byte blocks
		for (uint32_t i=0;i<5;++i)
		{
			fread(buffer, 8, 1, fp);
			printf(".");
			for (uint32_t j=0;j<8;++j)
				printf("%.2X ", buffer[i]);
		}
		// Read non-adjacent blocks
		for (uint32_t i=0;i<5;++i)
		{
			fseek(fp, i*32, SEEK_SET);
			printf("*");
			fread(buffer, 8, 1, fp);
			printf("!");
			for (uint32_t j=0;j<8;++j)
				printf("%.2X ", buffer[i]);
		}
		delete [] buffer;
		fclose(fp);
	}
	else
		printf("File's not there.\r\n");

	return 0;
}
