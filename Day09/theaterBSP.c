#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>
#include	<utlist.h>
#define	CLAY_IMPLEMENTATION
#include	"UILib/UIStuff.h"
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/GameCamera.h"
#include	"UtilityLib/UpdateTimer.h"
#include	"InputLib/Input.h"
#include	"MaterialLib/StuffKeeper.h"
#include	"MaterialLib/Material.h"
#include	"MaterialLib/CBKeeper.h"
#include	"MaterialLib/PostProcess.h"
#include	"UtilityLib/PrimFactory.h"
#include	"UtilityLib/MiscStuff.h"


//macro for supplying fontid and size
#define	FONTDEETS(ui, fid)	.fontId = fid, .fontSize = UI_GetFontSize(ui, fid)

//Warm Autumn Glow
Clay_Color	sWAG0	={	0x00, 0x30, 0x49, 0xFF	};
Clay_Color	sWAG1	={	0xD6, 0x28, 0x28, 0xFF	};
Clay_Color	sWAG2	={	0xF7, 0x7F, 0x00, 0xFF	};
Clay_Color	sWAG3	={	0xFC, 0xBF, 0x49, 0xFF	};
Clay_Color	sWAG4	={	0xEA, 0xE2, 0xB7, 0xFF	};
Clay_Color	sClear	={	0xEA, 0xE2, 0xB7, 0x00	};

//this tree only has 4 plane normals
//0 is 1 0
//1 is 0 1
//2 is -1 0
//3 is 0 -1;
#define	POSX	0
#define	POSY	1
#define	NEGX	2
#define	NEGY	3

#define	NUM_RED_TILES		496
#define	RESX				1280
#define	RESY				720
#define	MAX_UI_VERTS		(8192 * 3)	//bump this up as needed (debug needs alot)
#define	FONT_SIZE_TINY		8
#define	FONT_SIZE_MEDIUM	16
#define	FONT_SIZE_BIG		40
#define	MOUSE_TO_ANG		0.001f
#define	KEYTURN_RATE		0.01f
#define	MOVE_RATE			100.0f
#define	RAY_WIDTH			50.0f


//input context, stuff input handlers will need
typedef struct	TestStuff_t
{
	GraphicsDevice	*mpGD;
	UIStuff			*mpUI;
	GameCamera		*mpCam;
	PrimObject		*mpManyRays;

	//misc data
	vec3	mLightDir;
	vec3	mEyePos;
	float	mDeltaYaw, mDeltaPitch;

	//toggles
	bool	mbLeftMouseDown;
	bool	mbRunning;
	bool	mbMouseLooking;
	
	//clay pointer stuff
	Clay_Vector2	mScrollDelta;
	Clay_Vector2	mMousePos;

	//font ids for various sizes
	//these will vary as the game assets vary
	uint16_t	mFontIDTiny;
	uint16_t	mFontIDMedium;
	uint16_t	mFontIDBig;

}	TestStuff;

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

//for drawing
typedef struct	SegList_t
{
	int64_t	mSegAX;
	int64_t	mSegAY;
	int64_t	mSegBX;
	int64_t	mSegBY;

	//list stuff
	struct SegList_t	*next, *prev;
}	SegList;


//static forward decs
static void	sSetupKeyBinds(Input *pInp);
static void	sSetupRastVP(GraphicsDevice *pGD);

//bsp theater stuff
static void		sSplitSegment(int planeType, int dist,
					int64_t aX, int64_t aY,
					int64_t bX, int64_t bY,
					int64_t *pFrontX, int64_t *pFrontY,
					int64_t *pBackX, int64_t *pBackY,
					int64_t *pSplitX, int64_t *pSplitY);
static int		sGetPlaneType(int64_t aX, int64_t aY, int64_t bX, int64_t bY,
					int64_t cX, int64_t cY, int64_t dX, int64_t dY);
static int		sGetPlaneSide(int planeType, int dist, int64_t x, int64_t y);
static void		sInsertNode(BSPNode **ppTree, BSPNode *pNode);
static int		sGetSide(BSPNode *pNode, BSPNode *pOtherNode);
static bool		sIsPlaneX(int64_t aX, int64_t aY, int64_t bX, int64_t bY);
static int64_t	sIDot(int64_t aX, int64_t aY, int64_t bX, int64_t bY);
static bool		sbIsQuadConvex(int64_t aX, int64_t aY, int64_t bX, int64_t bY,
					int64_t cX, int64_t cY, int64_t dX, int64_t dY);
static void		sGetSeggz(BSPNode *pNode, SegList **ppSegList);

//clay stuff
static const Clay_RenderCommandArray sCreateLayout(TestStuff *pTS, const StuffKeeper *pSK);
static void sHandleClayErrors(Clay_ErrorData errorData);

//material setups
static Material	*sMakeRayMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*sMakeSkyBoxMat(TestStuff *pTS, const StuffKeeper *pSK);

//input event handlers
static void	sLeftMouseDownEH(void *pContext, const SDL_Event *pEvt);
static void	sLeftMouseUpEH(void *pContext, const SDL_Event *pEvt);
static void	sRightMouseDownEH(void *pContext, const SDL_Event *pEvt);
static void	sRightMouseUpEH(void *pContext, const SDL_Event *pEvt);
static void sMouseMoveEH(void *pContext, const SDL_Event *pEvt);
static void MouseWheelEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveForwardEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveBackEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveLeftEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveRightEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveUpEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveDownEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyTurnLeftEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyTurnRightEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyTurnUpEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyTurnDownEH(void *pContext, const SDL_Event *pEvt);
static void sEscEH(void *pContext, const SDL_Event *pEvt);


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
		sInsertNode(&spRoot, pNode);
	}

	//store a bunch of vars in a struct
	//for ref/modifying by input handlers
	TestStuff	*pTS	=malloc(sizeof(TestStuff));
	memset(pTS, 0, sizeof(TestStuff));

	//input and key / mouse bindings
	Input	*pInput	=INP_CreateInput();
	sSetupKeyBinds(pInput);

	bool	bGDInit	=GD_Init(&pTS->mpGD, "Elves Playground!",
				0, 0, RESX, RESY, true, D3D_FEATURE_LEVEL_11_1);
	if(!bGDInit)
	{
		printf("Graphics init failed!\n");
		return	EXIT_FAILURE;
	}

	//turn on border
	GD_SetWindowBordered(pTS->mpGD, true);

	sSetupRastVP(pTS->mpGD);
	StuffKeeper	*pSK	=StuffKeeper_Create(pTS->mpGD);
	if(pSK == NULL)
	{
		printf("Couldn't create StuffKeeper!\n");
		GD_Destroy(&pTS->mpGD);
		return	EXIT_FAILURE;
	}

	CBKeeper	*pCBK	=CBK_Create(pTS->mpGD);
	PostProcess	*pPP	=PP_Create(pTS->mpGD, pSK, pCBK);

	PP_SetTargets(pPP, pTS->mpGD, "BackColor", "BackDepth");

	//these need align
	__attribute((aligned(32)))	mat4	charMat, camProj;
	__attribute((aligned(32)))	mat4	textProj;

	float	aspect	=(float)RESX / (float)RESY;

	pTS->mEyePos[0] =185.0f;
	pTS->mEyePos[1] =36.0f;
	pTS->mEyePos[2] =180.0f;

	//set sky gradient
	{
		vec3	skyHorizon	={	0.0f, 0.5f, 1.0f	};
		vec3	skyHigh		={	0.0f, 0.25f, 1.0f	};

		CBK_SetSky(pCBK, skyHorizon, skyHigh);
		CBK_SetFogVars(pCBK, 5000.0f, 30000.0f, true);
	}

	//sky
	PrimObject	*pSkyCube	=PF_CreateCube(20.0f, true, pTS->mpGD);

	//game camera
	pTS->mpCam	=GameCam_Create(false, 10.0f, 500000.0f, GLM_PI_4f, aspect, 1.0f, 10.0f);

	//3D Projection
	GameCam_GetProjection(pTS->mpCam, camProj);
	CBK_SetProjection(pCBK, camProj);

	//2d projection for text
	glm_ortho(0, RESX, RESY, 0, -1.0f, 1.0f, textProj);

	//set constant buffers to shaders, think I just have to do this once
	CBK_SetCommonCBToShaders(pCBK, pTS->mpGD);

	GD_OMSetBlendState(pTS->mpGD, StuffKeeper_GetBlendState(pSK, "NoBlending"));
	GD_OMSetDepthStencilState(pTS->mpGD, StuffKeeper_GetDepthStencilState(pSK, "EnableDepth"));
	GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pSK, "PointClamp"), 0);

	//set proj for 2D
	CBK_SetProjection(pCBK, textProj);
	CBK_UpdateFrame(pCBK, pTS->mpGD);

	pTS->mpUI	=UI_Create(pTS->mpGD, pSK, MAX_UI_VERTS);

	UI_AddAllFonts(pTS->mpUI);

	//pick out 3 useful ids
	pTS->mFontIDTiny	=UI_GetNearestFontSize(pTS->mpUI, FONT_SIZE_TINY);
	pTS->mFontIDMedium	=UI_GetNearestFontSize(pTS->mpUI, FONT_SIZE_MEDIUM);
	pTS->mFontIDBig		=UI_GetNearestFontSize(pTS->mpUI, FONT_SIZE_BIG);

	//clay init
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(clayMemory, (Clay_Dimensions) { (float)RESX, (float)RESY }, (Clay_ErrorHandler) { sHandleClayErrors });
    Clay_SetMeasureTextFunction(UI_MeasureText, pTS->mpUI);

	//debug info uses font id zero
	//so if zero happens to be huge, this will be messed up
	Clay_SetDebugModeEnabled(false);

	pTS->mLightDir[0]		=0.3f;
	pTS->mLightDir[1]		=-0.7f;
	pTS->mLightDir[2]		=-0.5f;

	glm_vec3_normalize(pTS->mLightDir);
	glm_mat4_identity(charMat);

	//materials
	Material	*pRayMat	=sMakeRayMat(pTS, pSK);
	Material	*pSkyBoxMat	=sMakeSkyBoxMat(pTS, pSK);


	//gather segments from the bsp for test drawing
	SegList	*pTmp, *pSeggz	=NULL;
	int		numSeggz		=0;

	sGetSeggz(spRoot, &pSeggz);
	DL_COUNT(pSeggz, pTmp, numSeggz);

	//primfactory expects an array of startpoints, array of endpoints,
	//an array of colours, numrays, raywidth, and GD
	vec3	*pStarts	=malloc(sizeof(vec3) * numSeggz);
	vec3	*pEnds		=malloc(sizeof(vec3) * numSeggz);

	int	segCnt	=0;
	DL_FOREACH(pSeggz, pTmp)
	{
		int	randHeight	=rand() / 8888888;

		pStarts[segCnt][0]	=pTmp->mSegAX;
		pStarts[segCnt][1]	=randHeight;
		pStarts[segCnt][2]	=pTmp->mSegAY;

		pEnds[segCnt][0]	=pTmp->mSegBX;
		pEnds[segCnt][1]	=randHeight;
		pEnds[segCnt][2]	=pTmp->mSegBY;

		segCnt++;
	}

	pTS->mpManyRays	=PF_CreateManyRays(
		pStarts, pEnds, NULL, numSeggz, RAY_WIDTH, pTS->mpGD);

	UpdateTimer	*pUT	=UpdateTimer_Create(true, false);
//	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 6.944444f);	//144hz
	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 16.66666f);	//60hz

	float	maxDT	=0.0f;

	pTS->mbRunning	=true;
	while(pTS->mbRunning)
	{
		pTS->mDeltaYaw		=0.0f;
		pTS->mDeltaPitch	=0.0f;
		
		UpdateTimer_Stamp(pUT);
		for(float secDelta =UpdateTimer_GetUpdateDeltaSeconds(pUT);
			secDelta > 0.0f;
			secDelta =UpdateTimer_GetUpdateDeltaSeconds(pUT))
		{
			//do input here
			//move turn etc
			INP_Update(pInput, pTS);

			UpdateTimer_UpdateDone(pUT);
		}

		//update materials incase light changed
		MAT_SetLightDirection(pRayMat, pTS->mLightDir);
//		MAT_SetLightDirection(pCubeMat, pTS->mLightDir);

		//render update
		float	dt	=UpdateTimer_GetRenderUpdateDeltaSeconds(pUT);
		{
			if(dt > maxDT)
			{
				maxDT	=dt;
			}
		}

		//camera update
		GameCam_UpdateRotation(pTS->mpCam, pTS->mEyePos, pTS->mDeltaPitch,
								pTS->mDeltaYaw, 0.0f);

		//Set proj and rast and depth for 3D
		CBK_SetProjection(pCBK, camProj);
		GD_OMSetDepthStencilState(pTS->mpGD, StuffKeeper_GetDepthStencilState(pSK, "EnableDepth"));

		//set no blend, I think post processing turns it on maybe
		GD_OMSetBlendState(pTS->mpGD, StuffKeeper_GetBlendState(pSK, "NoBlending"));
		GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pSK, "PointWrap"), 0);

		//set CB view
		{
			__attribute((aligned(32)))	mat4	viewMat;
			GameCam_GetViewMatrixFly(pTS->mpCam, viewMat, pTS->mEyePos);

			CBK_SetView(pCBK, viewMat, pTS->mEyePos);

			//set the skybox world mat to match eye pos
			glm_translate_make(viewMat, pTS->mEyePos);
			MAT_SetWorld(pSkyBoxMat, viewMat);
		}

		PP_SetTargets(pPP, pTS->mpGD, "BackColor", "BackDepth");
		PP_ClearDepth(pPP, pTS->mpGD, "BackDepth");
		PP_ClearTarget(pPP, pTS->mpGD, "BackColor");

		//update frame CB
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		//turn depth off for sky
		GD_OMSetDepthStencilState(pTS->mpGD, StuffKeeper_GetDepthStencilState(pSK, "DisableDepth"));

		//draw sky first
		GD_IASetIndexBuffers(pTS->mpGD, pSkyCube->mpIB, DXGI_FORMAT_R16_UINT, 0);
		GD_VSSetSRV(pTS->mpGD, pSkyCube->mpVBSRV, 0);

		MAT_Apply(pSkyBoxMat, pCBK, pTS->mpGD);
		GD_DrawIndexed(pTS->mpGD, pSkyCube->mIndexCount, 0, 0);

		//turn depth back on
		GD_OMSetDepthStencilState(pTS->mpGD, StuffKeeper_GetDepthStencilState(pSK, "EnableDepth"));

		//draw 3d stuff
		//draw rays
		GD_IASetIndexBuffers(pTS->mpGD, pTS->mpManyRays->mpIB, DXGI_FORMAT_R32_UINT, 0);
		GD_VSSetSRV(pTS->mpGD, pTS->mpManyRays->mpVBSRV, 0);
		MAT_Apply(pRayMat, pCBK, pTS->mpGD);
		GD_DrawIndexed(pTS->mpGD, pTS->mpManyRays->mIndexCount, 0, 0);

		//draw Nodes
//		GD_IASetIndexBuffers(pTS->mpGD, pTS->mpManyCubes->mpIB, DXGI_FORMAT_R32_UINT, 0);
//		GD_VSSetSRV(pTS->mpGD, pTS->mpManyCubes->mpVBSRV, 0);
//		MAT_Apply(pCubeMat, pCBK, pTS->mpGD);
//		GD_DrawIndexed(pTS->mpGD, pTS->mpManyCubes->mIndexCount, 0, 0);

		//set proj for 2D
		CBK_SetProjection(pCBK, textProj);
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		Clay_UpdateScrollContainers(true, pTS->mScrollDelta, dt);

		pTS->mScrollDelta.x	=pTS->mScrollDelta.y	=0.0f;
	
		Clay_RenderCommandArray renderCommands = sCreateLayout(pTS, pSK);
	
		UI_BeginDraw(pTS->mpUI);
	
		UI_ClayRender(pTS->mpUI, renderCommands);
	
		UI_EndDraw(pTS->mpUI);
	
		GD_Present(pTS->mpGD);
	}

	GD_Destroy(&pTS->mpGD);

	return	EXIT_SUCCESS;
}


//statics
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
	assert(false);
	return	0;
}

static void	sGetSeggz(BSPNode *pNode, SegList **ppSegList)
{
	assert(ppSegList);

	SegList	*pNodeSeg	=malloc(sizeof(SegList));

	pNodeSeg->mSegAX	=pNode->mSegAX;
	pNodeSeg->mSegAY	=pNode->mSegAY;
	pNodeSeg->mSegBX	=pNode->mSegBX;
	pNodeSeg->mSegBY	=pNode->mSegBY;

	pNodeSeg->next	=pNodeSeg->prev	=NULL;

	DL_PREPEND(*ppSegList, pNodeSeg);

	if(pNode->mpBack != NULL)
	{
		sGetSeggz(pNode->mpBack, ppSegList);
	}
	if(pNode->mpFront != NULL)
	{
		sGetSeggz(pNode->mpFront, ppSegList);
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

static void	sInsertNode(BSPNode **ppTree, BSPNode *pNode)
{
	assert(ppTree);

	BSPNode	*pTree	=*ppTree;

	if(pTree == NULL)
	{
		*ppTree	=pNode;
		return;
	}

	int	side	=sGetSide(pTree, pNode);

	if(side == 0)
	{
		sInsertNode(&pTree->mpBack, pNode);
	}
	else if(side == 1)
	{
		sInsertNode(&pTree->mpFront, pNode);
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

		sInsertNode(&pTree->mpBack, pNode);
		sInsertNode(&pTree->mpFront, pFront);
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


static Material	*sMakeRayMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	vec3	light0		={	1.0f, 1.0f, 1.0f		};
	vec3	light1		={	0.5f, 0.5f, 0.5f		};
	vec3	light2		={	0.2f, 0.2f, 0.2f		};
	vec4	ghosty		={	1.0f, 1.0f, 1.0f, 0.5f	};
	vec3	lessShiny	={	0.25f, 0.25f, 0.25f		};

	MAT_SetLights(pRet, light0, light1, light2, pTS->mLightDir);
	MAT_SetVShader(pRet, "StaticVS", pSK);
	MAT_SetPShader(pRet, "TriColorPS", pSK);
	MAT_SetSolidColour(pRet, ghosty);
	MAT_SetSpecular(pRet, lessShiny, 6.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}

static Material	*sMakeSkyBoxMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	MAT_SetVShader(pRet, "SkyBoxVS", pSK);
	MAT_SetPShader(pRet, "SkyGradientFogPS", pSK);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}


//event handlers (eh)
static void	sLeftMouseDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbLeftMouseDown	=true;

	if(!pTS->mbMouseLooking)
	{
		Clay_SetPointerState(pTS->mMousePos, true);
	}
}

static void	sLeftMouseUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbLeftMouseDown	=false;

	if(!pTS->mbMouseLooking)
	{
		Clay_SetPointerState(pTS->mMousePos, false);
	}
}

static void	sRightMouseDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;
	assert(pTS);

	GD_SetMouseRelative(pTS->mpGD, true);

	pTS->mbMouseLooking	=true;
}

static void	sRightMouseUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	GD_SetMouseRelative(pTS->mpGD, false);

	pTS->mbMouseLooking	=false;
}

static void	sMouseMoveEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	if(pTS->mbMouseLooking)
	{
		pTS->mDeltaYaw		+=(pEvt->motion.xrel * MOUSE_TO_ANG);
		pTS->mDeltaPitch	+=(pEvt->motion.yrel * MOUSE_TO_ANG);
	}
	else
	{
		pTS->mMousePos.x	=pEvt->motion.x;
		pTS->mMousePos.y	=pEvt->motion.y;

		Clay_SetPointerState(pTS->mMousePos, pTS->mbLeftMouseDown);
	}
}

static void	MouseWheelEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mScrollDelta.x	=pEvt->wheel.x;
	pTS->mScrollDelta.y	=pEvt->wheel.y;
}

static void	sKeyMoveForwardEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	forward;
	GameCam_GetForwardVec(pTS->mpCam, forward);
	glm_vec3_scale(forward, MOVE_RATE, forward);

	glm_vec3_add(pTS->mEyePos, forward, pTS->mEyePos);
}

static void	sKeyMoveBackEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	forward;
	GameCam_GetForwardVec(pTS->mpCam, forward);
	glm_vec3_scale(forward, MOVE_RATE, forward);

	glm_vec3_sub(pTS->mEyePos, forward, pTS->mEyePos);
}

static void	sKeyMoveLeftEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	right;
	GameCam_GetRightVec(pTS->mpCam, right);
	glm_vec3_scale(right, MOVE_RATE, right);

	glm_vec3_sub(pTS->mEyePos, right, pTS->mEyePos);
}

static void	sKeyMoveRightEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	right;
	GameCam_GetRightVec(pTS->mpCam, right);
	glm_vec3_scale(right, MOVE_RATE, right);

	glm_vec3_add(pTS->mEyePos, right, pTS->mEyePos);
}

static void	sKeyMoveUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	up;
	GameCam_GetUpVec(pTS->mpCam, up);
	glm_vec3_scale(up, MOVE_RATE, up);

	glm_vec3_add(pTS->mEyePos, up, pTS->mEyePos);
}

static void	sKeyMoveDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	up;
	GameCam_GetUpVec(pTS->mpCam, up);
	glm_vec3_scale(up, MOVE_RATE, up);

	glm_vec3_sub(pTS->mEyePos, up, pTS->mEyePos);
}

static void	sKeyTurnLeftEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaYaw	-=KEYTURN_RATE;
}

static void	sKeyTurnRightEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaYaw	+=KEYTURN_RATE;
}

static void	sKeyTurnUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaPitch	+=KEYTURN_RATE;
}

static void	sKeyTurnDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaPitch	-=KEYTURN_RATE;
}

static void	sEscEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbRunning	=false;
}


static void	sSetupKeyBinds(Input *pInp)
{
	//event style bindings
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_ESCAPE, sEscEH);

	//held bindings
	//movement
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_W, sKeyMoveForwardEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_A, sKeyMoveLeftEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_S, sKeyMoveBackEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_D, sKeyMoveRightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_SPACE, sKeyMoveUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_Z, sKeyMoveDownEH);

	//key turning
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_Q, sKeyTurnLeftEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_E, sKeyTurnRightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_R, sKeyTurnUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_T, sKeyTurnDownEH);

	//move data events
	INP_MakeBinding(pInp, INP_BIND_TYPE_MOVE, SDL_EVENT_MOUSE_MOTION, sMouseMoveEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_MOVE, SDL_EVENT_MOUSE_WHEEL, MouseWheelEH);

	//down/up events
	INP_MakeBinding(pInp, INP_BIND_TYPE_PRESS, SDL_BUTTON_RIGHT, sRightMouseDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_RELEASE, SDL_BUTTON_RIGHT, sRightMouseUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_PRESS, SDL_BUTTON_LEFT, sLeftMouseDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_RELEASE, SDL_BUTTON_LEFT, sLeftMouseUpEH);
}

static void	sSetupRastVP(GraphicsDevice *pGD)
{
	D3D11_RASTERIZER_DESC	rastDesc;
	rastDesc.AntialiasedLineEnable	=false;
	rastDesc.CullMode				=D3D11_CULL_BACK;
	rastDesc.FillMode				=D3D11_FILL_SOLID;
	rastDesc.FrontCounterClockwise	=true;
	rastDesc.MultisampleEnable		=false;
	rastDesc.DepthBias				=0;
	rastDesc.DepthBiasClamp			=0;
	rastDesc.DepthClipEnable		=true;
	rastDesc.ScissorEnable			=false;
	rastDesc.SlopeScaledDepthBias	=0;
	ID3D11RasterizerState	*pRast	=GD_CreateRasterizerState(pGD, &rastDesc);

	D3D11_VIEWPORT	vp;

	vp.Width	=RESX;
	vp.Height	=RESY;
	vp.MaxDepth	=1.0f;
	vp.MinDepth	=0.0f;
	vp.TopLeftX	=0;
	vp.TopLeftY	=0;

	GD_RSSetViewPort(pGD, &vp);
	GD_RSSetState(pGD, pRast);
	GD_IASetPrimitiveTopology(pGD, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}


static Clay_RenderCommandArray	sCreateLayout(TestStuff *pTS, const StuffKeeper *pSK)
{
	Clay_BeginLayout();

	CLAY(CLAY_ID("OuterContainer"), { .layout =
		{
			.layoutDirection = CLAY_TOP_TO_BOTTOM,
			.sizing =
			{
				.width = CLAY_SIZING_GROW(0),
				.height = CLAY_SIZING_GROW(0)
			},
			.padding = { 8, 8, 8, 8 },
			.childGap = 8
		},
		.backgroundColor	=sClear,})
	{
		CLAY(CLAY_ID("Instruct"), {	.layout	=
			{
				.layoutDirection = CLAY_TOP_TO_BOTTOM,
				.sizing =
				{
					.width = CLAY_SIZING_FIT(0),
					.height = CLAY_SIZING_PERCENT(0.08f)
				},
				.padding = { 2, 2, 2, 2 },
				.childGap = 2
			},
			.backgroundColor	=sWAG3,})
		{
			CLAY_TEXT(CLAY_STRING("Visualizing theater..."),
				CLAY_TEXT_CONFIG({
					FONTDEETS(pTS->mpUI, pTS->mFontIDTiny),
					.textColor = sWAG0 }));
		}
	}

	return	Clay_EndLayout();
}

static bool reinitializeClay = false;

static void sHandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
    if (errorData.errorType == CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED) {
        reinitializeClay = true;
        Clay_SetMaxElementCount(Clay_GetMaxElementCount() * 2);
    } else if (errorData.errorType == CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED) {
        reinitializeClay = true;
        Clay_SetMaxMeasureTextCacheWordCount(Clay_GetMaxMeasureTextCacheWordCount() * 2);
    }
}