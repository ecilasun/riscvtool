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
		int *a = (int*)malloc(512);
		for (int i=0;i<512;++i)
			a[i] = i*i;
	}

	return 0;
}
