#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<assert.h>
#include	<string.h>


//grab two digits that form the biggest joltage
static int	sFindLargestJoltageTwo(const char *szBank, int len)
{
	int		joltage		=0;
	char	szNum[4]	={0};
	for(int j=0;j < len;j++)
	{
		if(szBank[j] == 0)
		{
			//null terminator
			break;
		}
		if(szBank[j] == '\n')
		{
			break;
		}
		for(int i=0;i < len;i++)
		{
			if(szBank[i] == 0)
			{
				//null terminator
				break;
			}
			if(szBank[i] == '\n')
			{
				break;
			}
			if(i == j)
			{
				continue;
			}

			if(j > i)
			{
				szNum[1]	=szBank[j];
				szNum[0]	=szBank[i];
			}
			else
			{
				szNum[0]	=szBank[j];
				szNum[1]	=szBank[i];
			}

			int	jolt	=atoi(szNum);

			if(jolt > joltage)
			{
				joltage	=jolt;
			}
		}
	}
	return	joltage;
}


static int sCompareInts(const void *a, const void *b)
{
	int	*pA	=a;
	int	*pB	=b;

	return	(*pA - *pB);
}

//comedy brute force
//grab twelve digits that form the biggest joltage
//this will never work but I want to see how bad it is
static uint64_t	sFindLargestJoltageTwelve(const char *szBank, int len)
{
	uint64_t	joltage		=0;
	char		szNum[13]	={0};
	for(int a=0;a < len;a++)
	{
		if(szBank[a] == 0 || szBank[a] == '\n')
		{
			break;
		}
		for(int b=0;b < len;b++)
		{
			if(a == b)
			{
				continue;
			}
			if(szBank[b] == 0 || szBank[b] == '\n')
			{
				break;
			}
			for(int c=0;c < len;c++)
			{
				if(a == c || b == c)
				{
					continue;
				}
				if(szBank[c] == 0 || szBank[c] == '\n')
				{
					break;
				}
				for(int d=0;d < len;d++)
				{
					if(a == d || b == d || c == d)
					{
						continue;
					}
					if(szBank[d] == 0 || szBank[d] == '\n')
					{
						break;
					}
					for(int e=0;e < len;e++)
					{
						if(a == e || b == e || c == e || d == e)
						{
							continue;
						}
						if(szBank[e] == 0 || szBank[e] == '\n')
						{
							break;
						}
						for(int f=0;f < len;f++)
						{
							if(a == f || b == f || c == f || d == f || e == f)
							{
								continue;
							}
							if(szBank[f] == 0 || szBank[f] == '\n')
							{
								break;
							}
							for(int g=0;g < len;g++)
							{
								if(a == g || b == g || c == g || d == g || e == g || f == g)
								{
									continue;
								}
								if(szBank[g] == 0 || szBank[g] == '\n')
								{
									break;
								}
								for(int h=0;h < len;h++)
								{
									if(a == h || b == h || c == h || d == h || e == h || f == h || g == h)
									{
										continue;
									}
									if(szBank[h] == 0 || szBank[h] == '\n')
									{
										break;
									}
									for(int i=0;i < len;i++)
									{
										if(a == i || b == i || c == i || d == i || e == i || f == i || g == i || h == i)
										{
											continue;
										}
										if(szBank[i] == 0 || szBank[i] == '\n')
										{
											break;
										}
										for(int j=0;j < len;j++)
										{
											if(a == j || b == j || c == j || d == j || e == j || f == j || g == j || h == j || i == j)
											{
												continue;
											}
											if(szBank[j] == 0 || szBank[j] == '\n')
											{
												break;
											}
											for(int k=0;k < len;k++)
											{
												if(a == k || b == k || c == k || d == k || e == k || f == k || g == k || h == k || i == k || j == k)
												{
													continue;
												}
												if(szBank[k] == 0 || szBank[k] == '\n')
												{
													break;
												}
												for(int l=0;l < len;l++)
												{
													if(a == l || b == l || c == l || d == l || e == l || f == l || g == l || h == l || i == l || j == l || k == l)
													{
														continue;
													}
													if(szBank[l] == 0 || szBank[l] == '\n')
													{
														break;
													}

													//dump indexes into an array
													int	indexes[12]	={ a, b, c, d, e, f, g, h, i, j, k, l };

													qsort(indexes, 12, sizeof(int), sCompareInts);

													for(int idx=0;idx < 12;idx++)
													{
														szNum[idx]	=szBank[indexes[idx]];
													}

													uint64_t	jolt	=atoll(szNum);

													if(jolt > joltage)
													{
														joltage	=jolt;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return	joltage;
}

//better algo than brute force
//find highest digit in the string and its position
//ensure there are 11 remaining digits in the token
//find highest digit after first position
//ensure there are 10 remaining digits in the token

static char	sFindHighestDigit(const char *szTok, int pos, int digitsAfter, int *pHighPos)
{
	int	len	=strlen(szTok);

	//newline makes the len 1 longer
	if(szTok[len - 1] == '\n')
	{
		len--;
	}

	char	highest		=0;
	int		highestPos	=0;
	for(int i=pos + 1;i < (len - digitsAfter);i++)
	{
		if(szTok[i] == 0)
		{
			//null terminator
			break;
		}
		if(szTok[i] == '\n')
		{
			break;
		}

		if(szTok[i] > highest)
		{
			highest		=szTok[i];
			highestPos	=i;
		}
	}

	*pHighPos	=highestPos;

	return	highest;
}

#define	MAX_DIGITS	12


int main(int argc, char *argv[])
{
	printf("Calculating joltage...\n");

	//open the input file with all the battery banks
	FILE	*pInp	=fopen("input.txt", "r");
	if(pInp == NULL)
	{
		printf("Cannot open input file.\n");
		return	EXIT_FAILURE;
	}

	char		tokBuf[256]	={0};
	char		numBuf[13]	={0};
	uint64_t	total		=0;
	for(int i=0;;i++)
	{
		fgets(tokBuf, 255, pInp);

		int	highPos	=-1;
		for(int digit=0;digit < MAX_DIGITS;digit++)
		{
			char	high	=sFindHighestDigit(tokBuf, highPos, MAX_DIGITS - digit - 1, &highPos);

			numBuf[digit]	=high;
		}

		uint64_t	joltage	=atoll(numBuf);

		printf("Token %d has joltage %llu\n", i, joltage);

		total	+=joltage;

		if(feof(pInp))
		{
			break;
		}
	}	

	fclose(pInp);

	printf("Joltages add up to %llu\n", total);

	return	EXIT_SUCCESS;
}