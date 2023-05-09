#define _GNU_SOURCE 1

#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	if (argc <= 1)
		printf("Usage: dump filename\n");
	else
	{
		FILE *fp = fopen(argv[1], "rb");
		if (fp)
		{
			printf("\n");
			uint8_t *buffer = new uint8_t[16];
			int readsize = 0;
			do
			{
				readsize = fread(buffer, 1, 16, fp);

				for (uint32_t i=0; i<readsize; ++i)
					printf("%.2X ", buffer[i]);

				for (uint32_t i=0; i<readsize; ++i)
					if (buffer[i]>=32)
						printf("%c", buffer[i]);
					else
						printf(".");
				printf("\n");
			} while (readsize > 0);
			fclose(fp);
		}
		else
			printf("File '%s' not found, please use full path\n", argv[1]);
	}

	return 0;
}
