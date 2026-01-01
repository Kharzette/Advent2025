#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<assert.h>
#include	<string.h>


#define	MAX_OPERANDS	8
#define	MAX_PROBLEMS	2048

static int	sOperands[MAX_PROBLEMS][MAX_OPERANDS];

#define	MULTIPLY_OP	1
#define	ADD_OP		2
static uint8_t	sOperations[MAX_PROBLEMS];

static int	sProbCount, sOperandCount;


int main(int argc, char *argv[])
{
	printf("Bug Math!\n");

	FILE	*pInp	=fopen("input.txt", "r");
	if(pInp == NULL)
	{
		printf("Cannot open input file.\n");
		return	EXIT_FAILURE;
	}

	sProbCount	=sOperandCount	=0;

	//count up the lines, this should reveal the number of operands
	char	lineBuf[4096];
	for(;;)
	{
		fgets(lineBuf, 4095, pInp);
		if(feof(pInp))
		{
			break;
		}

		sOperandCount++;
	}

	//last line will be operatios

	//rewind
	fseek(pInp, SEEK_SET, 0);

	//alloc space to stuff the whole thing in memory
	char	*pData	=malloc(sOperandCount * 4096);
	memset(pData, 0, sOperandCount * 4096);

	for(int i=0;i < sOperandCount;i++)
	{
		fgets(&pData[(i * 4096)], 4095, pInp);
		if(feof(pInp))
		{
			assert(false && "a bit early!");
			break;
		}
	}

	char	numBuf[32];
	int		numLen	=0;
	int		curOper	=0;

	//read in operands right to left
	for(int j=4095;j >= 0;j--)
	{
		//check for a gap
		bool	bGap	=true;
		for(int i=0;i < sOperandCount;i++)
		{
			if(pData[(i * 4096) + j] != ' ')
			{
				bGap	=false;
			}
		}

		if(bGap)
		{
			//fill remaining slots with deadbeef
			//to show it is not in use
			for(int i=curOper;i < sOperandCount;i++)
			{
				sOperands[sProbCount][i]	=0xDEADBEEF;
			}
			curOper	=0;
			sProbCount++;
			continue;
		}

		//grab digits
		for(int i=0;i < sOperandCount;i++)
		{
			int	ofs	=i * 4096;

			//a digit?
			if(pData[ofs + j] >= 48 && pData[ofs + j] <= 57)
			{
				numBuf[numLen]	=pData[ofs + j];
				numLen++;
			}
		}

		if(numLen > 0)
		{
			//copy out a number
			numBuf[numLen]					=0;
			sOperands[sProbCount][curOper]	=atoi(numBuf);
			numLen	=0;
			curOper++;
		}
	}

	//fill remaining slots with deadbeef
	//to show it is not in use
	for(int i=curOper;i < sOperandCount;i++)
	{
		sOperands[sProbCount][i]	=0xDEADBEEF;
	}

	//might need this if the numbers are right up against the left
	sProbCount++;

	//test print the operands
	for(int p=0;p < sProbCount;p++)
	{
		printf("Problem %d: ", p);

		for(int o=0;o < sOperandCount;o++)
		{
			printf("%d, ", sOperands[p][o]);
		}

		printf("\n");
	}

	//grab operations
	int	operCount	=0;
	fgets(lineBuf, 4095, pInp);
	for(int j=4095;j >= 0;j--)
	{
		if(lineBuf[j] == '\n' || lineBuf[j] == 0)
		{
			continue;
		}

		if(lineBuf[j] == '*')
		{
			sOperations[operCount]	=MULTIPLY_OP;
			operCount++;
		}
		else if(lineBuf[j] == '+')
		{
			sOperations[operCount]	=ADD_OP;
			operCount++;
		}
	}

	assert(operCount == sProbCount);

	//do the problems
	uint64_t	total	=0;
	for(int i=0;i < sProbCount;i++)
	{
		if(sOperations[i] == ADD_OP)
		{
			uint64_t	answer	=0;
			for(int j=0;j < sOperandCount;j++)
			{
				int	operand	=sOperands[i][j];

				//skip borger
				if(operand != 0xDEADBEEF)
				{
					answer	+=sOperands[i][j];
				}
			}

			printf("Problem %d answer is: %llu\n", i, answer);

			total	+=answer;
		}
		else if(sOperations[i] == MULTIPLY_OP)
		{
			uint64_t	answer	=1;
			for(int j=0;j < sOperandCount;j++)
			{
				int	operand	=sOperands[i][j];

				//skip borger
				if(operand != 0xDEADBEEF)
				{
					answer	*=sOperands[i][j];
				}
			}
			
			printf("Problem %d answer is: %llu\n", i, answer);

			total	+=answer;
		}
		else
		{
			assert(false && "Unknown op!");
		}
	}

	fclose(pInp);

	printf("Total answer is %llu\n", total);

	return	EXIT_SUCCESS;
}