#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<assert.h>
#include	<string.h>


#define	MAX_RANGES	2048

static uint64_t	sRangeMins[MAX_RANGES];
static uint64_t	sRangeMaxs[MAX_RANGES];
static uint64_t	sRangeCount;


static bool	sbIsMergey(uint64_t id)
{
	for(int i=0;i < sRangeCount;i++)
	{
		if(id >= sRangeMins[i] && id <= sRangeMaxs[i])
		{
			return	true;
		}
	}
	return	false;
}


int main(int argc, char *argv[])
{
	printf("Fresh food checker!\n");

	FILE	*pInp	=fopen("input.txt", "r");
	if(pInp == NULL)
	{
		printf("Cannot open input file.\n");
		return	EXIT_FAILURE;
	}

	char	lineBuf[64];
	for(sRangeCount=0;;)
	{
		fgets(lineBuf, 63, pInp);

		char	*pDashPos	=strchr(lineBuf, '-');
		if(pDashPos == NULL)
		{
			//finished parsing ranges
			break;
		}

		//null the dash
		*pDashPos	=0;

		//advance to max pos
		pDashPos++;

		sRangeMins[sRangeCount]	=atoll(lineBuf);
		sRangeMaxs[sRangeCount]	=atoll(pDashPos);

		sRangeCount++;

		if(feof(pInp))
		{
			break;
		}
	}

	int	numFresh	=0;

	//now grab ids
	for(;;)
	{
		fgets(lineBuf, 63, pInp);

		uint64_t	id	=atoll(lineBuf);

		if(sbIsMergey(id))
		{
			numFresh++;
		}

		if(feof(pInp))
		{
			break;
		}
	}

	fclose(pInp);

	printf("%d fresh ingredients\n", numFresh);

	return	EXIT_SUCCESS;
}