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

static uint64_t	sMergedMins[MAX_RANGES];
static uint64_t	sMergedMaxs[MAX_RANGES];
static uint64_t	sMergedCount;


//check against merged
static bool	sbIsMergey(void)
{
	//see if any merged ranges overlap
	for(int i=0;i < sMergedCount;i++)
	{
		for(int j=0;j < sMergedCount;j++)
		{
			if(i == j)
			{
				continue;
			}
			if(sMergedMins[j] >= sMergedMins[i] && sMergedMins[j] <= sMergedMaxs[i])
			{
				return	true;
			}
			if(sMergedMaxs[j] >= sMergedMins[i] && sMergedMaxs[j] <= sMergedMaxs[i])
			{
				return	true;
			}
		}
	}
	return	false;
}

//check against merged
static bool	sMergeRange(uint64_t min, uint64_t max)
{
	assert(max >= min);

	int	numMerges	=0;
	for(int i=0;i < sMergedCount;i++)
	{
		//any overlap?
		if((min >= sMergedMins[i] && min <= sMergedMaxs[i])
			|| (max >= sMergedMins[i] && max <= sMergedMaxs[i]))
		{
			//expand merged range
			if(min < sMergedMins[i])
			{
				sMergedMins[i]	=min;
			}
			if(max > sMergedMaxs[i])
			{
				sMergedMaxs[i]	=max;
			}
			numMerges++;
		}
		//does i fit entirely inside range?
		else if(min < sMergedMins[i] && max > sMergedMaxs[i])
		{
			sMergedMins[i]	=min;
			sMergedMaxs[i]	=max;
			numMerges++;
		}
	}
	return	numMerges > 0;
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

		assert(sRangeMins[sRangeCount] <= sRangeMaxs[sRangeCount]);

		sRangeCount++;

		if(feof(pInp))
		{
			break;
		}
	}
	fclose(pInp);

	//merge the ranges so there is no overlap
	//copy range 0 to start
	sMergedCount	=0;
	sMergedMins[0]	=sRangeMins[0];
	sMergedMaxs[0]	=sRangeMaxs[0];
	sMergedCount++;

	for(;;)
	{
		for(int j=0;j < sRangeCount;j++)
		{
			if(!sMergeRange(sRangeMins[j], sRangeMaxs[j]))
			{
				sMergedMins[sMergedCount]	=sRangeMins[j];
				sMergedMaxs[sMergedCount]	=sRangeMaxs[j];
				sMergedCount++;
				continue;
			}
		}
		printf("%llu merged ranges\n", sMergedCount);

		//merges themselves might need merging
		if(sbIsMergey())
		{
			//copy merged into ranges and go again
			memcpy(sRangeMins, sMergedMins, sizeof(uint64_t) * sMergedCount);
			memcpy(sRangeMaxs, sMergedMaxs, sizeof(uint64_t) * sMergedCount);
			sRangeCount		=sMergedCount;
			sMergedCount	=0;
		}
		else
		{
			break;
		}
	}

	//count up ids in merged ranges
	uint64_t	numIDs	=0;
	for(int i=0;i < sMergedCount;i++)
	{
		assert(sMergedMaxs[i] >= sMergedMins[i]);

		numIDs	+=sMergedMaxs[i] - sMergedMins[i] + 1;
	}

	printf("%llu fresh ids\n", numIDs);

	return	EXIT_SUCCESS;
}