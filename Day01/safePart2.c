#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<assert.h>


static int	sTurnDial(int *pCurDialValue, int amount, bool bRight)
{
	int	clicksToZero	=0;
	int	curVal			=*pCurDialValue;

	for(int i=0;i < amount;i++)
	{
		if(bRight)
		{
			curVal++;
			if(curVal > 99)
			{
				curVal	-=100;
			}
		}
		else
		{
			curVal--;
			if(curVal < 0)
			{
				curVal	+=100;
			}
		}

		if(curVal == 0)
		{
			//click!
			clicksToZero++;
		}
	}

	*pCurDialValue	=curVal;

	return	clicksToZero;
}



int main(int argc, char *argv[])
{
	printf("Safe dial turning action!\n");

	int	dialPos		=50;	//dial starts pointing at 50
	int	dialZeros	=0;		//when the dial clicks to 0

	//open the input file with all the dial turnings
	FILE	*pInp	=fopen("input.txt", "r");
	if(pInp == NULL)
	{
		printf("Cannot open input file.\n");
		return	EXIT_FAILURE;
	}

	char	lineBuf[32];
	for(;;)
	{
		fgets(lineBuf, 30, pInp);

		int		turnAmount	=0;
		bool	bRight		=true;

		//left or right?
		if(lineBuf[0] == 'R')
		{
			bRight	=true;
		}
		else if(lineBuf[0] == 'L')
		{
			bRight	=false;
		}
		else
		{
			printf("Input file format error around %d!\n", ftell(pInp));
			return	EXIT_FAILURE;
		}

		//amount
		turnAmount	=atoi(lineBuf + 1);

		dialZeros	+=sTurnDial(&dialPos, turnAmount, bRight);

		if(feof(pInp))
		{
			break;
		}
	}	

	fclose(pInp);

	printf("Password is %d\n", dialZeros);

	return	EXIT_SUCCESS;
}