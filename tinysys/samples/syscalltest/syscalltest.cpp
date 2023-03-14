#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main()
{
	printf("This is fun!\r\n");

	FILE *fp = fopen("doesntexist.txt", "rb");
	if (fp)
	{
		printf("What doesn't, does.\r\n");
		fclose(fp);
	}
	else
	{
		printf("Welp, it's not there.\r\n");
	}

	return 0;
}
