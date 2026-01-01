#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<assert.h>
#include	<string.h>

#define	MAX_REPEATS	69


//read a token from pFile until a comma is reached
static bool	sReadCSToken(FILE *pFile, char *pDest, int size)
{
	int	pos	=0;
	for(;;)
	{
		int	character	=fgetc(pFile);

		pDest[pos]	=character;

		pos++;
		if(pos >= size)
		{
			return	false;
		}

		if(character == EOF)
		{
			return	false;
		}

		if(character == ',')
		{
			pDest[pos]	=0;
			break;
		}
	}

	return	true;
}

static bool	sGetIDRange(char *szTok, uint64_t *pStart, uint64_t *pEnd)
{
	//find pos of -
	char *pDashPos	=strchr(szTok, '-');
	if(pDashPos == NULL)
	{
		return	false;
	}

	int	dashPos	=pDashPos - szTok;

	//null -
	szTok[dashPos]	=0;

	//grab start
	*pStart	=atoll(szTok);

	//grab end
	*pEnd	=atoll(szTok + dashPos + 1);

	return	true;
}

static bool	sCheckTokenReps(const char *szTok, int len)
{
	bool	bMatch	=true;
	int		tokLen	=strlen(szTok);
	for(int ofs=len;ofs < tokLen;ofs+=len)
	{
		for(int i=0;i < len;i++)
		{
			if(szTok[i] != szTok[ofs + i])
			{
				bMatch	=false;
			}
		}
	}
	return	bMatch;
}

static bool	sTestID(uint64_t id)
{
	char	szID[64];

	snprintf(szID, 64, "%llu", id);

	int	len	=strlen(szID);

	for(int i=1;i <= (len/2);i++)
	{
		if(sCheckTokenReps(szID, i))
		{
			return	true;
		}
	}
	return	false;
}


int main(int argc, char *argv[])
{
	printf("Checking for bad ids from a silly elf...\n");

	uint64_t	invalids	=0;

	//open the input file with all the dial turnings
	FILE	*pInp	=fopen("input.txt", "r");
	if(pInp == NULL)
	{
		printf("Cannot open input file.\n");
		return	EXIT_FAILURE;
	}

	char	tokBuf[64];
	for(;;)
	{
		bool	bGood	=sReadCSToken(pInp, tokBuf, 64);

		uint64_t	start, end;

		sGetIDRange(tokBuf, &start, &end);

		printf("Token range %llu %llu\n", start, end);

		//test token range
		for(uint64_t i=start;i <= end;i++)
		{
			if(sTestID(i))
			{
				printf("Invalid token %llu found\n", i);
				invalids	+=i;
			}
		}

		if(!bGood)
		{
			break;
		}
	}	

	fclose(pInp);

	printf("Invalid tokens add up to %llu\n", invalids);

	return	EXIT_SUCCESS;
}