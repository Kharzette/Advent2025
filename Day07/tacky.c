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


//fire a beam from coordinates x, y
//beams always go downward
static void	sFireTachyonBeamu(const uint8_t *pData, uint8_t *pVData,
	int xDim, int yDim, int x, int y)
{
	//print a | to the view data if needed
	if(pData[(y * xDim) + x] == '.')
	{
		pVData[(y * xDim) + x]	='|';
		sPrintManifold(pVData, xDim, yDim);
	}

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
			return;
		}

		//print a | to the view data if needed
		if(pData[(y * xDim) + x] == '.')
		{
			pVData[(y * xDim) + x]	='|';
		}
		else if(pData[(y * xDim) + x] == '^')
		{
			pVData[(y * xDim) + x]	='+';	//plus for split beamu
		}
		sPrintManifold(pVData, xDim, yDim);

		//see what was struck
		if(pData[(y * xDim) + x] == '.')
		{
			continue;
		}

		if(pData[(y * xDim) + x] == '^')
		{
			//splitter!
			//create 2 new beamuz if no beam already there
			if(pVData[(y * xDim) + (x - 1)] == '.')
			{
//				getchar();
				sFireTachyonBeamu(pData, pVData, xDim, yDim, x - 1, y);
			}

			if(pVData[(y * xDim) + (x + 1)] == '.')
			{
//				getchar();
				sFireTachyonBeamu(pData, pVData, xDim, yDim, x + 1, y);
			}

			//this beamu ends
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
	uint8_t	*pMData	=malloc(lineLen * lineCount);
	uint8_t	*pVData	=malloc(lineLen * lineCount);

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

	memcpy(pVData, pMData, lineLen * lineCount);

	//find emitter and fire beamu!
	for(int y=0;y < lineCount;y++)
	{
		for(int x=0;x < lineLen;x++)
		{
			if(pMData[(y * lineLen) + x] == 'S')
			{
				sFireTachyonBeamu(pMData, pVData, lineLen, lineCount, x, y);
			}
		}
	}

	//count the plusses in the view data
	int	numSplits	=0;
	for(int y=0;y < lineCount;y++)
	{
		for(int x=0;x < lineLen;x++)
		{
			if(pVData[(y * lineLen) + x] == '+')
			{
				numSplits++;
			}
		}
	}

	printf("Beamu split %d times.\n", numSplits);

	return	EXIT_SUCCESS;
}