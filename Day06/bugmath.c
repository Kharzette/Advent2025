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

	//read in operands
	for(int i=0;i < sOperandCount;i++)
	{
		fgets(lineBuf, 4095, pInp);
		if(feof(pInp))
		{
			assert(false && "a bit early!");
			break;
		}

		//grab numbers
		char	numBuf[32];
		int		numLen	=0;
		int		numProb	=0;
		for(int j=0;j < 4095;j++)
		{
			if(lineBuf[j] == '\n' || lineBuf[j] == 0)
			{
				if(numLen > 0)
				{
					//copy out a number
					numBuf[numLen]			=0;
					sOperands[numProb][i]	=atoi(numBuf);
					numProb++;
					numLen	=0;
				}
				break;
			}
			else if(lineBuf[j] >= 48 && lineBuf[j] <= 57)
			{
				numBuf[numLen]	=lineBuf[j];
				numLen++;
			}
			else
			{
				if(numLen > 0)
				{
					//copy out a number
					numBuf[numLen]			=0;
					sOperands[numProb][i]	=atoi(numBuf);
					numProb++;
					numLen	=0;
				}
			}
		}

		if(sProbCount == 0)
		{
			sProbCount	=numProb;
		}

		assert(sProbCount == numProb);
	}

	//test print the operands
	for(int o=0;o < sOperandCount;o++)
	{
		printf("Operands %d: ", o);

		for(int p=0;p < sProbCount;p++)
		{
			printf("%d, ", sOperands[p][o]);
		}

		printf("\n");
	}

	//grab operations
	int	operCount	=0;
	fgets(lineBuf, 4095, pInp);
	for(int j=0;j < 4095;j++)
	{
		if(lineBuf[j] == '\n' || lineBuf[j] == 0)
		{
			break;
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
				answer	+=sOperands[i][j];
			}

			total	+=answer;
		}
		else if(sOperations[i] == MULTIPLY_OP)
		{
			uint64_t	answer	=1;
			for(int j=0;j < sOperandCount;j++)
			{
				answer	*=sOperands[i][j];
			}

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