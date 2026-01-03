#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>


uint64_t	sComputeArea(uint64_t aX, uint64_t aY, uint64_t bX, uint64_t bY)
{
	if(aX == bX || aY == bY)
	{
		return	0;
	}

	//find x length
	uint64_t	xLen	=0;
	if(aX > bX)
	{
		xLen	=aX - bX;
	}
	else
	{
		xLen	=bX - aX;
	}

	//find y length
	uint64_t	yLen	=0;
	if(aY > bY)
	{
		yLen	=aY - bY;
	}
	else
	{
		yLen	=bY - aY;
	}

	//add 1 to lens
	xLen++;
	yLen++;

	return	xLen * yLen;
}

int main(int argc, char *argv[])
{
	printf("Movie room!\n");

	FILE	*pInp	=fopen("input.txt", "r");
	if(pInp == NULL)
	{
		printf("Cannot open input file.\n");
		return	EXIT_FAILURE;
	}

	char	lineBuf[16];
	int		numLines	=0;
	for(;;)
	{
		fgets(lineBuf, 15, pInp);

		numLines++;

		if(feof(pInp))
		{
			break;
		}
	}	

	fseek(pInp, 0, SEEK_SET);

	//big room!
	uint64_t	*redX	=malloc(sizeof(uint64_t) * numLines);
	uint64_t	*redY	=malloc(sizeof(uint64_t) * numLines);

	//read red tile coordinates
	for(int i=0;i < numLines;i++)
	{
		fgets(lineBuf, 15, pInp);

		redX[i]	=atoll(lineBuf);

		char	*pComma	=strchr(lineBuf, ',');

		redY[i]	=atoll(pComma + 1);
	}

	fclose(pInp);

	//brute first, see how slow it goes
	uint64_t	biggest	=0;
	int			bigA	=-1;
	int			bigB	=-1;
	for(int i=0;i < numLines;i++)
	{
		for(int j=0;j < numLines;j++)
		{
			if(i == j)
			{
				continue;
			}

			uint64_t	area	=sComputeArea(redX[i], redY[i], redX[j], redY[j]);
			if(area > biggest)
			{
				biggest	=area;
				bigA	=i;
				bigB	=j;
			}
		}
	}

	printf("Biggest rect is %llu in size made by index %d and %d.\n", biggest, bigA, bigB);

	return	EXIT_SUCCESS;
}