#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>


int main(int argc, char *argv[])
{
	printf("Forklift action\n");

	int	totalRolls	=0;

	//open the input file with the warehouse map
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

	printf("Warehouse is %d by %d.\n", lineLen, lineCount);

	uint8_t	*pWHData	=malloc(lineLen * lineCount);

	//rewind
	fseek(pInp, SEEK_SET, 0);
	for(int y=0;y < lineCount;y++)
	{
		//read 2D style
		fread(&pWHData[y * lineLen], lineLen, sizeof(uint8_t), pInp);

		//skip \n
		fseek(pInp, SEEK_CUR, 1);
	}

	fclose(pInp);

	//a gpu would be so good at this
	//print to test data is good
/*	for(int y=0;y < lineCount;y++)
	{
		for(int x=0;x < lineLen;x++)
		{
			putc(pWHData[(y * lineLen) + x], stdout);
		}
		putc('\n', stdout);
	}
*/
	//check the rolls
	for(int y=0;y < lineCount;y++)
	{
		for(int x=0;x < lineLen;x++)
		{
			int	yOfs	=y * lineLen;

			if(pWHData[yOfs + x] != '@')
			{
				putc(pWHData[yOfs + x], stdout);
				continue;
			}

			int	nearbyRolls	=0;

			if(y > 0)
			{
				if(x > 0)
				{
					//check upper left
					if(pWHData[((y - 1) * lineLen) + (x - 1)] == '@')
					{
						nearbyRolls++;
					}
				}

				//check up
				if(pWHData[((y - 1) * lineLen) + x] == '@')
				{
					nearbyRolls++;
				}

				if(x < (lineLen - 1))
				{
					//check upper right
					if(pWHData[((y - 1) * lineLen) + (x + 1)] == '@')
					{
						nearbyRolls++;
					}
				}
			}

			if(x > 0)
			{
				//check left
				if(pWHData[yOfs + (x - 1)] == '@')
				{
					nearbyRolls++;
				}
			}

			if(x < (lineLen - 1))
			{
				//check right
				if(pWHData[yOfs + (x + 1)] == '@')
				{
					nearbyRolls++;
				}
			}

			if(y < (lineCount - 1))
			{
				if(x > 0)
				{
					//check lower left
					if(pWHData[((y + 1) * lineLen) + (x - 1)] == '@')
					{
						nearbyRolls++;
					}
				}

				//check down
				if(pWHData[((y + 1) * lineLen) + x] == '@')
				{
					nearbyRolls++;
				}

				if(x < (lineLen - 1))
				{
					//check lower right
					if(pWHData[((y + 1) * lineLen) + (x + 1)] == '@')
					{
						nearbyRolls++;
					}
				}
			}

			if(nearbyRolls < 4)
			{
				putc('x', stdout);
				totalRolls++;
			}
			else
			{
				putc(pWHData[yOfs + x], stdout);
			}
		}
		putc('\n', stdout);
	}

	printf("%d rolls of paper can be grabbed by forklift.\n", totalRolls);

	return	EXIT_SUCCESS;
}