#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<assert.h>


int main(int argc, char *argv[])
{
	printf("Safe dial turning action!\n");

	int	dialPos		=50;	//dial starts pointing at 50
	int	dialZeros	=0;		//when the dial points at 0

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

		if(turnAmount > 99)
		{
			int	gack	=69;
			gack++;
		}

		//spinnnnnnn
		if(bRight)
		{
			dialPos	+=turnAmount;
		}
		else
		{
			dialPos	-=turnAmount;
		}

		//wrap if needed
		while(dialPos > 99)
		{
			dialPos	-=100;
		}
		while(dialPos < 0)
		{
			dialPos	=99 - ~dialPos;
		}

		assert(dialPos >= 0 && dialPos <= 99);

		//check for a zero
		if(dialPos == 0)
		{
			dialZeros++;
		}

		if(feof(pInp))
		{
			break;
		}
	}	

	fclose(pInp);

	printf("Password is %d\n", dialZeros);

	return	EXIT_SUCCESS;
}