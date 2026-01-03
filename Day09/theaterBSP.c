#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>
#include	<utlist.h>


#define	NUM_RED_TILES	496

//this tree only has 4 plane normals
//0 is 1 0
//1 is 0 1
//2 is -1 0
//3 is 0 -1;
#define	POSX	0
#define	POSY	1
#define	NEGX	2
#define	NEGY	3


typedef struct	BSPNode_t
{
	int	mPlaneType;
	int	mPlaneDist;

	int64_t	mSegAX;
	int64_t	mSegAY;
	int64_t	mSegBX;
	int64_t	mSegBY;

	struct BSPNode_t	*mpFront, *mpBack;

}	BSPNode;


static BSPNode	*spRoot	=NULL;


//split segment ab returning front back and split verts
static void	sSplitSegment(int planeType, int dist,
	int64_t aX, int64_t aY,
	int64_t bX, int64_t bY,
	int64_t *pFrontX, int64_t *pFrontY,
	int64_t *pBackX, int64_t *pBackY,
	int64_t *pSplitX, int64_t *pSplitY)
{
	if(planeType == POSX)
	{
		if(aX > dist)
		{
			assert(bX <= dist);

			*pFrontX	=aX;
			*pBackX		=bX;
			*pSplitX	=dist;
		}
		else if(aX < dist)
		{
			assert(bX >= dist);

			*pFrontX	=bX;
			*pBackX		=aX;
			*pSplitX	=dist;
		}
		else
		{
			assert(bX != dist);

			if(bX > dist)
			{
				*pFrontX	=bX;
				*pBackX		=aX;
				*pSplitX	=dist;
			}
		}
		*pFrontY	=*pBackY	=*pSplitY	=aY;
	}
	else if(planeType == NEGX)
	{
		if(aX > -dist)
		{
			assert(bX <= -dist);

			*pFrontX	=bX;
			*pBackX		=aX;
			*pSplitX	=-dist;
		}
		else if(aX < -dist)
		{
			assert(bX >= -dist);

			*pFrontX	=aX;
			*pBackX		=bX;
			*pSplitX	=-dist;
		}
		else
		{
			assert(bX != -dist);

			if(bX > -dist)
			{
				*pFrontX	=aX;
				*pBackX		=bX;
				*pSplitX	=-dist;
			}
		}
		*pFrontY	=*pBackY	=*pSplitY	=aY;
	}
	else if(planeType == POSY)
	{
		if(aY > dist)
		{
			assert(bY <= dist);

			*pFrontY	=aY;
			*pBackY		=bY;
			*pSplitY	=dist;
		}
		else if(aY < dist)
		{
			assert(bY >= dist);

			*pFrontY	=bY;
			*pBackY		=aY;
			*pSplitY	=dist;
		}
		else
		{
			assert(bY != dist);

			if(bY > dist)
			{
				*pFrontY	=bY;
				*pBackY		=aY;
				*pSplitY	=dist;
			}
		}
		*pFrontX	=*pBackX	=*pSplitX	=aX;
	}
	else if(planeType == NEGY)
	{
		if(aY > -dist)
		{
			assert(bY <= -dist);

			*pFrontY	=bY;
			*pBackY		=aY;
			*pSplitY	=-dist;
		}
		else if(aY < -dist)
		{
			assert(bY >= -dist);

			*pFrontY	=aY;
			*pBackY		=bY;
			*pSplitY	=-dist;
		}
		else
		{
			assert(bY != -dist);

			if(bY > -dist)
			{
				*pFrontY	=aY;
				*pBackY		=bY;
				*pSplitY	=-dist;
			}
		}
		*pFrontX	=*pBackX	=*pSplitX	=aX;
	}
}


//determine which side of node other is on
//0 for back, 1 for front, 2 for on
static int	sGetPlaneSide(int planeType, int dist, int64_t x, int64_t y)
{
	if(planeType == POSX)
	{
		if(x < dist)
		{
			return	0;
		}
		else if(x > dist)
		{
			return	1;
		}
		return	2;
	}
	else if(planeType == NEGX)
	{
		if(x < -dist)
		{
			return	1;
		}
		else if(x > -dist)
		{
			return	0;
		}
		return	2;
	}
	else if(planeType == POSY)
	{
		if(y < dist)
		{
			return	0;
		}
		else if(y > dist)
		{
			return	1;
		}
		return	2;
	}
	else if(planeType == NEGY)
	{
		if(y < -dist)
		{
			return	1;
		}
		else if(y > -dist)
		{
			return	0;
		}
		return	2;
	}
}

//determine which side of node other is on
//0 for back, 1 for front, 2 for split
static int	sGetSide(BSPNode *pNode, BSPNode *pOtherNode)
{
	int	A	=sGetPlaneSide(pNode->mPlaneType, pNode->mPlaneDist, pOtherNode->mSegAX, pOtherNode->mSegAY);
	int	B	=sGetPlaneSide(pNode->mPlaneType, pNode->mPlaneDist, pOtherNode->mSegBX, pOtherNode->mSegBY);

	if(A == 0 && B == 0)
	{
		return	0;
	}
	else if(A == 1 && B == 1)
	{
		return	1;
	}
	else if(A == 0 && B == 2)
	{
		return	0;	//mostly behind, touching
	}
	else if(A == 2 && B == 0)
	{
		return	0;	//mostly behind, touching
	}
	else if(A == 1 && B == 2)
	{
		return	1;	//mostly front, touching
	}
	else if(A == 2 && B == 1)
	{
		return	1;	//mostly front, touching
	}
	else if(A == 2 && B == 2)
	{
		//this one is tricksy, not sure
		printf("sGetSide: segment on plane\n");
		return	1;
	}
	return	2;
}


static void	sInsertNode(BSPNode *pTree, BSPNode *pNode)
{
	if(pTree == NULL)
	{
		pTree	=pNode;
		return;
	}

	int	side	=sGetSide(pTree, pNode);

	if(side == 0)
	{
		sInsertNode(pTree->mpBack, pNode);
	}
	else if(side == 1)
	{
		sInsertNode(pTree->mpFront, pNode);
	}
	else
	{
		int64_t	frontX, backX, splitX;
		int64_t	frontY, backY, splitY;

		//split
		sSplitSegment(pTree->mPlaneType, pTree->mPlaneDist,
			pNode->mSegAX, pNode->mSegAY, pNode->mSegBX, pNode->mSegBY,
			&frontX, &frontY, &backX, &backY, &splitX, &splitY);

		BSPNode	*pFront	=malloc(sizeof(BSPNode));

		pFront->mpBack	=pFront->mpFront	=NULL;
		pFront->mPlaneDist	=pNode->mPlaneDist;
		pFront->mPlaneType	=pNode->mPlaneType;
		pFront->mSegAX		=splitX;
		pFront->mSegAY		=splitY;
		pFront->mSegBX		=frontX;
		pFront->mSegBY		=frontY;

		pNode->mSegAX	=backX;
		pNode->mSegAY	=backY;
		pNode->mSegBX	=splitX;
		pNode->mSegBY	=splitY;

		sInsertNode(pTree->mpBack, pNode);
		sInsertNode(pTree->mpFront, pFront);
	}


}

static bool	sIsPlaneX(int64_t aX, int64_t aY, int64_t bX, int64_t bY)
{
	if(aX == bX)
	{
		//y plane
		return	false;
	}
	else if(aY == bY)
	{
		//x plane
		return	true;
	}

	assert(false && "rule breaker!");

	return	false;
}

//get the plane type for AB with extra convex verts C and D
static int	sGetPlaneType(int64_t aX, int64_t aY, int64_t bX, int64_t bY,
	int64_t cX, int64_t cY, int64_t dX, int64_t dY)
{
	if(sIsPlaneX(aX, aY, bX, bY))
	{
		//x plane type
		if(cX > aX)
		{
			assert(dX >= aX);

			return	NEGX;
		}
		else if(cX < aX)
		{
			assert(dX <= aX);

			return	POSX;
		}
		else
		{
			assert(false && "Colinear!");
		}
	}
	else
	{
		//y plane type
		if(cY > aY)
		{
			assert(dY >= aY);

			return	POSY;
		}
		else if(cY < aY)
		{
			assert(dY <= aY);

			return	NEGY;
		}
		else
		{
			assert(false && "Colinear!");
		}
	}
}

static int64_t	sIDot(int64_t aX, int64_t aY, int64_t bX, int64_t bY)
{
	return	aX * bX + aY * bY;
}

//determine if the quad formed by the lines AB, BC, BD, DA is convex
static bool	sbIsQuadConvex(int64_t aX, int64_t aY, int64_t bX, int64_t bY,
	int64_t cX, int64_t cY, int64_t dX, int64_t dY)
{
	//is AB an X or Y plane?
	if(sIsPlaneX(aX, aY, bX, bY))
	{
		//x plane
		//make sure C and D are on the same side
		if(cX > aX)
		{
			if(dX < aX)
			{
				return	false;
			}
		}
	}
	else
	{
		//y plane
		//make sure C and D are on the same side
		if(cY > aY)
		{
			if(dY < aY)
			{
				return	false;
			}
		}
	}

	//is BC an X or Y plane?
	if(sIsPlaneX(bX, bY, cX, cY))
	{
		//x plane
		//make sure A and D are on the same side
		if(dX > bX)
		{
			if(aX < bX)
			{
				return	false;
			}
		}
	}
	else
	{
		//y plane
		//make sure A and D are on the same side
		if(dY > bY)
		{
			if(aY < bY)
			{
				return	false;
			}
		}
	}

	//is AD an X or Y plane?
	if(sIsPlaneX(aX, aY, dX, dY))
	{
		//x plane
		//make sure B and C are on the same side
		if(bX > aX)
		{
			if(cX < aX)
			{
				return	false;
			}
		}
	}
	else
	{
		//y plane
		//make sure B and C are on the same side
		if(bY > aY)
		{
			if(cY < aY)
			{
				return	false;
			}
		}
	}
	return	true;
}

int main(int argc, char *argv[])
{
	printf("Movie room doom!\n");

	FILE	*pInp	=fopen("input.txt", "r");
	if(pInp == NULL)
	{
		printf("Cannot open input file.\n");
		return	EXIT_FAILURE;
	}

	char	lineBuf[16];
	int		numLines	=0;
	for(;;)
	{
		fgets(lineBuf, 15, pInp);

		numLines++;

		if(feof(pInp))
		{
			break;
		}
	}	

	fseek(pInp, 0, SEEK_SET);

	//big room!
	uint64_t	*redX	=malloc(sizeof(uint64_t) * numLines);
	uint64_t	*redY	=malloc(sizeof(uint64_t) * numLines);

	//read red tile coordinates
	for(int i=0;i < numLines;i++)
	{
		fgets(lineBuf, 15, pInp);

		redX[i]	=atoll(lineBuf);

		char	*pComma	=strchr(lineBuf, ',');

		redY[i]	=atoll(pComma + 1);
	}

	fclose(pInp);

	//was thinking of building a 2D bsp tree, but
	//the tricky part is how to define the plane of
	//each segment?

	//I think if segment index is a side of a quad,
	//then seg index + is a side, seg index - 1 is a side,
	//then the fourth side is drawn between, if the quad
	//is non convex, skip, otherwise add the planes
	for(int i=0;i < numLines;i++)
	{
		int	prevIdx		=i - 1;
		int	nextIdx		=i + 1;
		int	nexNexIdx	=i + 2;

		//wrap
		if(prevIdx < 0)
		{
			prevIdx	=numLines - 1;
		}
		if(nextIdx >= numLines)
		{
			nextIdx	=0;
		}
		if(nexNexIdx >= numLines)
		{
			nexNexIdx	-=numLines;
		}

		int64_t	segAX	=redX[i];
		int64_t	segAY	=redY[i];
		int64_t	segBX	=redX[nextIdx];
		int64_t	segBY	=redY[nextIdx];
		int64_t	segCX	=redX[nexNexIdx];
		int64_t	segCY	=redY[nexNexIdx];
		int64_t	segDX	=redX[prevIdx];
		int64_t	segDY	=redY[prevIdx];

		if(!sbIsQuadConvex(segAX, segAY, segBX, segBY, segCX, segCY, segDX, segDY))
		{
			continue;
		}

		//form AB plane
		BSPNode	*pNode	=malloc(sizeof(BSPNode));

		pNode->mpBack		=NULL;
		pNode->mpFront		=NULL;
		pNode->mPlaneType	=sGetPlaneType(segAX, segAY, segBX, segBY, segCX, segCY, segDX, segDY);
		pNode->mSegAX		=segAX;
		pNode->mSegAY		=segAY;
		pNode->mSegBX		=segBX;
		pNode->mSegBY		=segBY;

		//insert AB into bsp
		sInsertNode(spRoot, pNode);
	}

	return	EXIT_SUCCESS;
}