#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>
#include	<time.h>


static void	sPrintManifold(const uint8_t *pData, int xDim, int yDim)
{
	for(int y=0;y < yDim;y++)
	{
		for(int x=0;x < xDim;x++)
		{
			putc(pData[(y * xDim) + x], stdout);
		}
		putc('\n', stdout);
	}
}

static void	sPrintManifoldNumbers(const uint64_t *pData, int xDim, int yDim)
{
	for(int y=0;y < yDim;y++)
	{
		for(int x=0;x < xDim;x++)
		{
			printf("%03d", pData[(y * xDim) + x]);
		}
		putc('\n', stdout);
	}
}

//new idea, consider y the time variable
//move all particles in all timeless at once
static void	sMarchParticles(const uint8_t *pData, uint64_t *pVData,
	int xDim, int yDim, uint64_t *pNumTimeLines)
{
	//find the tachyon source
	int	srcX	=-1;
	for(int x=0;x < xDim;x++)
	{
		//convert view data to just a number of rays in the spot
		if(pData[x] == 'S')
		{
			srcX		=x;
			pVData[x]	=1;	//mark it
			break;
		}
	}

	assert(srcX >= 0);

	for(int y=1;y < yDim;y++)
	{
		int	yOfs		=y * xDim;
		int	prevYOfs	=(y - 1) * xDim;

//		sPrintManifoldNumbers(pVData, xDim, yDim);
//		printf("\n");

		for(int x=0;x < xDim;x++)
		{
			uint64_t	valUp	=pVData[prevYOfs + x];
			if(valUp == 0)
			{
				continue;
			}

			assert(valUp < 0xFFFFFFFFFFFFFF);

			//continue the particle path in time
			if(pData[yOfs + x] == '.')
			{
				pVData[yOfs + x]	+=valUp;
			}
			else if(pData[yOfs + x] == '^')
			{
				//split
				pVData[yOfs + (x - 1)]	+=valUp;
				pVData[yOfs + (x + 1)]	+=valUp;

				//timelines diverge
				(*pNumTimeLines)	+=valUp;
			}
		}
	}

//	sPrintManifoldNumbers(pVData, xDim, yDim);
//	printf("\n");

	//check against energy values on the bottom row
	uint64_t	val		=0;
	int			yOfs	=(yDim - 1) * xDim;
	for(int x=0;x < xDim;x++)
	{
		val	+=pVData[yOfs + x];
	}
	assert(val == *pNumTimeLines);
}

//track progress
static int	sXProgress	=0;

//fire a beam from coordinates x, y
//beams always go downward
static void	sFireTachyonBeamu(const uint8_t *pData, uint8_t *pVData,
	int xDim, int yDim, int x, int y, uint64_t *pNumTimeLines)
{
	//print a | to the view data if needed
//	if(pData[(y * xDim) + x] == '.')
//	{
//		pVData[(y * xDim) + x]	='|';
//		sPrintManifold(pVData, xDim, yDim);
//	}

	//pause for showing tree as it grows
//	struct timespec	pausu	={ 0 };
//	struct timespec	remaining;

//	pausu.tv_nsec	=1000000L;

//	nanosleep(&pausu, &remaining);

	for(;;)
	{
		//advance beamu!
		y++;
		if(y >= yDim)
		{
			if(x > sXProgress)
			{
				sXProgress	=x;
				printf("X: %d\n", x);			
			}
			return;
		}

		//print a | to the view data if needed
//		if(pData[(y * xDim) + x] == '.')
//		{
//			pVData[(y * xDim) + x]	='|';
//		}
//		else if(pData[(y * xDim) + x] == '^')
//		{
//			pVData[(y * xDim) + x]	='+';	//plus for split beamu
//		}
//		sPrintManifold(pVData, xDim, yDim);

		//see what was struck
		if(pData[(y * xDim) + x] == '.')
		{
			continue;
		}

		if(pData[(y * xDim) + x] == '^')
		{
			//splitter!
			//create 2 new beamuz if no beam already there
//			if(pVData[(y * xDim) + (x - 1)] == '.')
			{
//				getchar();
				sFireTachyonBeamu(pData, pVData, xDim, yDim, x - 1, y, pNumTimeLines);
			}

//			if(pVData[(y * xDim) + (x + 1)] == '.')
			(*pNumTimeLines)++;
			{
//				getchar();
				sFireTachyonBeamu(pData, pVData, xDim, yDim, x + 1, y, pNumTimeLines);
			}

			return;
		}
	}
}


int main(int argc, char *argv[])
{
	printf("Tachyons?  That's tacky!\n");

	//open the input file with beamu splitters
	FILE	*pInp	=fopen("input.txt", "r");
	if(pInp == NULL)
	{
		printf("Cannot open input file.\n");
		return	EXIT_FAILURE;
	}

	//count the lines
	char	lineBuf[256];
	int		lineCount	=0;
	int		lineLen		=0;
	for(lineCount=0;;lineCount++)
	{
		fgets(lineBuf, 255, pInp);

//		printf("Line: %d: %s", lineCount, lineBuf);
		int	len	=strlen(lineBuf);

		//strip newline if there
		if(lineBuf[len - 1] == '\n')
		{
			len--;
		}

		//find the longest line, in case the data isn't a perfect square
		if(len > lineLen)
		{
			lineLen	=len;
		}

		if(feof(pInp))
		{
			if(len > 1)
			{
				lineCount++;	//count this line if it has stuff
			}
			break;
		}
	}

	printf("Manifold is %d by %d.\n", lineLen, lineCount);

	//alloc manifold space plus view space for printing
	uint8_t		*pMData	=malloc(lineLen * lineCount);
	uint64_t	*pVData	=malloc(lineLen * lineCount * sizeof(uint64_t));

	//rewind
	fseek(pInp, SEEK_SET, 0);
	for(int y=0;y < lineCount;y++)
	{
		//read 2D style
		fread(&pMData[y * lineLen], lineLen, sizeof(uint8_t), pInp);

		//skip \n
		fseek(pInp, SEEK_CUR, 1);
	}

	fclose(pInp);

//	memcpy(pVData, pMData, lineLen * lineCount);
	memset(pVData, 0, lineLen * lineCount * sizeof(uint64_t));

	//find emitter and fire beamu!
	uint64_t	numTimeLines	=1;
	sMarchParticles(pMData, pVData, lineLen, lineCount, &numTimeLines);
	/*
	for(int y=0;y < lineCount;y++)
	{
		for(int x=0;x < lineLen;x++)
		{
			if(pMData[(y * lineLen) + x] == 'S')
			{
				sFireTachyonBeamu(pMData, pVData, lineLen, lineCount, x, y, &numTimeLines);
			}
		}
	}*/

	printf("Number of timelines %llu\n", numTimeLines);

	return	EXIT_SUCCESS;
}