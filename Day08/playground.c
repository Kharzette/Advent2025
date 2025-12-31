#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>
#include	<cglm/call.h>
#include	<utlist.h>
#include	<assert.h>
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

//10 for test data, 1000 for real data
#define	NUM_CONNEXIONS		1000
#define	NUM_CIRCUITS		NUM_CONNEXIONS
#define	RESX				1280
#define	RESY				720
#define	MAX_UI_VERTS		(8192 * 3)	//bump this up as needed (debug needs alot)
#define	FONT_SIZE_TINY		8
#define	FONT_SIZE_MEDIUM	16
#define	FONT_SIZE_BIG		40
#define	MOUSE_TO_ANG		0.001f
#define	KEYTURN_RATE		0.01f
#define	MOVE_RATE			100.0f
#define	RAY_WIDTH			100.0f


//input context, stuff input handlers will need
typedef struct	TestStuff_t
{
	GraphicsDevice	*mpGD;
	UIStuff			*mpUI;
	GameCamera		*mpCam;
	PrimObject		*mpManyRays;
	PrimObject		*mpManyCubes;

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

typedef struct	JunxBox_t
{
	//3d position in the playground cube volume
	vec3	mPos;
	int		mID;

	//circuit info if any
	int	mCircuit;
	int	mConnectedTo;

	//list stuff
	struct JunxBox_t	*next, *prev;
}	JunxBox;

//array of lists for holding circuits
static JunxBox	*sCircuits[NUM_CIRCUITS];
static int		sTotalConnexions;

//temp space to hold nearest calcs
int		sNearestIdx[NUM_CIRCUITS];
float	sNearestDist[NUM_CIRCUITS];

//static forward decs
static void	sSetupKeyBinds(Input *pInp);
static void	sSetupRastVP(GraphicsDevice *pGD);

//clay stuff
static const Clay_RenderCommandArray sCreateLayout(TestStuff *pTS, const StuffKeeper *pSK);
static void sHandleClayErrors(Clay_ErrorData errorData);

//material setups
static Material	*sMakeRayMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*sMakeCubeMat(TestStuff *pTS, const StuffKeeper *pSK);
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

//stuff for the playground
static void	sPrintCircuit(int cIdx);
static void	sAddToCircuit(int cIdx, JunxBox *pBox);
static void sMergeCircuits(int A, int B);
static int	sFindEmptyCircuit();
static void sTestCircuits();
static bool	sConnect(JunxBox *pJB, JunxBox *pTJB);
static void	sGrabVec(const char *pLine, JunxBox *pJB);


int main(int argc, char *argv[])
{
	printf("Crimmus lights!\n");

	FILE	*pInp	=fopen("input.txt", "r");
	if(pInp == NULL)
	{
		printf("Cannot open input file.\n");
		return	EXIT_FAILURE;
	}

	char	lineBuf[32];
	int		numLines	=0;
	for(;;)
	{
		//count up the lines
		fgets(lineBuf, 30, pInp);

		numLines++;

		if(feof(pInp))
		{
			break;
		}
	}

	JunxBox	*pJunxBoxen	=malloc(sizeof(JunxBox) * numLines);

	fseek(pInp, SEEK_SET, 0);
	for(int i=0;i < numLines;i++)
	{
		//count up the lines
		fgets(lineBuf, 30, pInp);

		sGrabVec(lineBuf, &pJunxBoxen[i]);

		//set ID
		pJunxBoxen[i].mID			=i;
		pJunxBoxen[i].mConnectedTo	=-1;

		if(feof(pInp))
		{
			break;
		}
	}

	fclose(pInp);

	//make connexions!
	sTotalConnexions	=0;

	//start with every box in a solo circuit
	for(int i=0;i < numLines;i++)
	{
		sAddToCircuit(i, &pJunxBoxen[i]);

		pJunxBoxen[i].mCircuit	=i;
	}

	while(true)
	{
		//clear near data
		for(int x=0;x < numLines;x++)
		{
			sNearestIdx[x]	=-1;
			sNearestDist[x]	=FLT_MAX;
		}

		for(int i=0;i < numLines;i++)
		{
			//connected already?
			if(pJunxBoxen[i].mConnectedTo >= 0)
			{
				//skip
				continue;
			}

			float	minDist	=FLT_MAX;
			int		nearest	=-1;

			//find the nearest junc boxen to i
			//for test data brute force, but might
			//need spatial partitioning for real data
			for(int j=0;j < numLines;j++)
			{
				if(i == j)
				{
					continue;
				}

				//already connected to i?
				if(pJunxBoxen[j].mConnectedTo == i)
				{
					continue;
				}

				//see if the bigger numbers might be causing problems
//				double	abx	=pJunxBoxen[i].mPos[0] - pJunxBoxen[j].mPos[0];
//				double	aby	=pJunxBoxen[i].mPos[1] - pJunxBoxen[j].mPos[1];
//				double	abz	=pJunxBoxen[i].mPos[2] - pJunxBoxen[j].mPos[2];

//				double	dist	=sqrt((abx * abx) + (aby * aby) + (abz * abz));
				float	dist	=glm_vec3_distance(pJunxBoxen[i].mPos, pJunxBoxen[j].mPos);

//				double	diff	=fabs(dist - dist2);

//				if(diff > 0.1)
//				{
//					printf("Difference %f\n", diff);
//				}

				if(dist < minDist)
				{
					minDist	=dist;
					nearest	=j;
				}

				sNearestIdx[i]	=nearest;
				sNearestDist[i]	=minDist;
			}
		}

		//connect shortest connexion
		float	shortDist	=FLT_MAX;
		int		shortIdx	=-1;
		for(int i=0;i < numLines;i++)
		{
			if(sNearestDist[i] < shortDist)
			{
				shortDist	=sNearestDist[i];
				shortIdx	=i;
			}
		}

		assert(shortIdx != -1);

		sConnect(&pJunxBoxen[shortIdx], &pJunxBoxen[sNearestIdx[shortIdx]]);

		sTestCircuits();

		//test connected in same circuit
		for(int i=0;i < numLines;i++)
		{
			if(pJunxBoxen[i].mConnectedTo == -1)
			{
				continue;
			}

			if(pJunxBoxen[pJunxBoxen[i].mConnectedTo].mCircuit
				!= pJunxBoxen[i].mCircuit)
			{
				printf("Connected in different circuit! %d %d\n", i, pJunxBoxen[i].mConnectedTo);
			}
		}

		if(sTotalConnexions >= NUM_CONNEXIONS)
		{
			break;
		}
	}

	//test for unconnected junxboxen
	for(int i=0;i < numLines;i++)
	{
		if(pJunxBoxen[i].mConnectedTo == -1)
		{
			printf("Box %d is unconnected!\n", i);
		}
	}

	//print circuits
	for(int i=0;i < NUM_CIRCUITS;i++)
	{
		if(sCircuits[i] == NULL)
		{
			continue;
		}

		sPrintCircuit(i);
	}

	//find the top 3 circuits
	int	topCons	=-1;
	int	topIdx	=-1;
	for(int i=0;i < NUM_CIRCUITS;i++)
	{
		int		cnt	=0;
		JunxBox	*pTmp;
		DL_COUNT(sCircuits[i], pTmp, cnt);

		if(cnt > topCons)
		{
			topCons	=cnt;
			topIdx	=i;
		}
	}

	int	top			=topIdx;
	int	topBoxen	=topCons;
	printf("Top circuit is %d with %d boxen.\n", top, topCons);

	topCons	=-1;
	topIdx	=-1;
	for(int i=0;i < NUM_CIRCUITS;i++)
	{
		if(i == top)
		{
			continue;
		}

		int		cnt	=0;
		JunxBox	*pTmp;
		DL_COUNT(sCircuits[i], pTmp, cnt);

		if(cnt > topCons)
		{
			topCons	=cnt;
			topIdx	=i;
		}
	}

	int	second		=topIdx;
	int	secondBoxen	=topCons;
	printf("Second circuit is %d with %d boxen.\n", second, topCons);

	topCons	=-1;
	topIdx	=-1;
	for(int i=0;i < NUM_CIRCUITS;i++)
	{
		if(i == top || i == second)
		{
			continue;
		}

		int		cnt	=0;
		JunxBox	*pTmp;
		DL_COUNT(sCircuits[i], pTmp, cnt);

		if(cnt > topCons)
		{
			topCons	=cnt;
			topIdx	=i;
		}
	}

	int	third		=topIdx;
	int	thirdBoxen	=topCons;
	printf("Third circuit is %d with %d boxen.\n", third, topCons);

	printf("Muld together they are %d\n", topBoxen * secondBoxen * thirdBoxen);

	sPrintCircuit(top);
	sPrintCircuit(second);
	sPrintCircuit(third);

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
	Material	*pCubeMat	=sMakeCubeMat(pTS, pSK);
	Material	*pSkyBoxMat	=sMakeSkyBoxMat(pTS, pSK);

	//make a big prim for the Christmas lights
	int	curRay	=0;

	//colours for circuits
	vec4	*pCircuitCols	=malloc(sizeof(vec4) * NUM_CIRCUITS);
	for(int i=0;i < NUM_CIRCUITS;i++)
	{
		Misc_RandomColour(pCircuitCols[i]);
	}

	//primfactory expects an array of startpoints, array of endpoints,
	//an array of colours, numrays, raywidth, and GD
	vec3	*pStarts	=malloc(sizeof(vec3) * sTotalConnexions);
	vec3	*pEnds		=malloc(sizeof(vec3) * sTotalConnexions);
	vec4	*pCols		=malloc(sizeof(vec4) * sTotalConnexions);

	for(int i=0;i < NUM_CIRCUITS;i++)
	{
		if(pJunxBoxen[i].mConnectedTo < 0)
		{
			continue;
		}

		glm_vec3_copy(pJunxBoxen[i].mPos, pStarts[curRay]);
		glm_vec3_copy(pJunxBoxen[pJunxBoxen[i].mConnectedTo].mPos, pEnds[curRay]);
		glm_vec4_copy(pCircuitCols[pJunxBoxen[i].mCircuit], pCols[curRay]);

		curRay++;
	}

	pTS->mpManyRays	=PF_CreateManyRays(pStarts, pEnds, pCols, curRay, RAY_WIDTH, pTS->mpGD);

	free(pEnds);
	free(pStarts);

	//for the nodes, pull these from pJunxBoxen, and maybe
	//try pulling from the circuits and see if they are different
	pStarts	=malloc(sizeof(vec3) * NUM_CIRCUITS);
	for(int i=0;i < NUM_CIRCUITS;i++)
	{
		glm_vec3_copy(pJunxBoxen[i].mPos, pStarts[i]);
	}

	pTS->mpManyCubes	=PF_CreateManyCubes(pStarts, pCols, NUM_CIRCUITS, RAY_WIDTH * 4.0f, pTS->mpGD);

	free(pStarts);
	free(pCols);

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
		MAT_SetLightDirection(pCubeMat, pTS->mLightDir);

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
		GD_IASetIndexBuffers(pTS->mpGD, pTS->mpManyCubes->mpIB, DXGI_FORMAT_R32_UINT, 0);
		GD_VSSetSRV(pTS->mpGD, pTS->mpManyCubes->mpVBSRV, 0);
		MAT_Apply(pCubeMat, pCBK, pTS->mpGD);
		GD_DrawIndexed(pTS->mpGD, pTS->mpManyCubes->mIndexCount, 0, 0);

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

	free(pJunxBoxen);

	return	EXIT_SUCCESS;
}


static void	sPrintCircuit(int cIdx)
{
	printf("Circuit %d:", cIdx);

	JunxBox	*pTmp;
	DL_FOREACH(sCircuits[cIdx], pTmp)
	{
		printf(" %4d:%5.0f,%5.0f,%5.0f", pTmp->mID,
			pTmp->mPos[0], pTmp->mPos[1], pTmp->mPos[2]);
	}
	printf("\n");
}

static void	sAddToCircuit(int cIdx, JunxBox *pBox)
{
	assert(pBox->mCircuit == -1);
	assert(cIdx < NUM_CIRCUITS);

	DL_PREPEND(sCircuits[cIdx], pBox);

	pBox->mCircuit	=cIdx;
}

static void sMergeCircuits(int A, int B)
{
	assert(A < B);
	assert(A < NUM_CIRCUITS);
	assert(B < NUM_CIRCUITS);

	//turn these on for merge details
//	printf("Merging: %d with %d\n", A, B);
//	sPrintCircuit(A);
//	sPrintCircuit(B);

	DL_CONCAT(sCircuits[A], sCircuits[B]);

//	printf("Merged:\n");
//	sPrintCircuit(A);

	//fix up the mCircuit vars
	JunxBox	*pTmp;
	DL_FOREACH(sCircuits[A], pTmp)
	{
		pTmp->mCircuit	=A;
	}

	sCircuits[B]	=NULL;
}

static int	sFindEmptyCircuit()
{
	for(int i=0;i < NUM_CIRCUITS;i++)
	{
		if(sCircuits[i] == NULL)
		{
			return	i;
		}
	}
	return	-1;
}

static void sTestCircuits()
{
	for(int i=0;i < NUM_CIRCUITS;i++)
	{
		if(sCircuits[i] == NULL)
		{
			continue;
		}

		int		cnt	=0;
		JunxBox	*pTmp;
		DL_COUNT(sCircuits[i], pTmp, cnt);

		if(cnt < 2)
		{
			printf("Circuit %d has 1 item!\n", i);
		}

		DL_FOREACH(sCircuits[i], pTmp)
		{
			if(pTmp->mCircuit != i)
			{
				printf("Bad mCircuit in circuit %d!\n", i);
			}
			if(pTmp->mConnectedTo == pTmp->mID)
			{
				printf("Connected to self in circuit %d!", i);
			}
		}
	}
}

static bool	sConnect(JunxBox *pJB, JunxBox *pTJB)
{
	//connect the passed in junxes
	assert(pJB->mConnectedTo == -1);
	
	//see if already connected
	if(pTJB->mConnectedTo == pJB->mID)
	{
		//already connected!
		printf("Boxen already hooxd up!\n");
		return	false;
	}

	printf("Connecting %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f dist: %.3f\n",
		pJB->mPos[0], pJB->mPos[1], pJB->mPos[2], 
		pTJB->mPos[0], pTJB->mPos[1], pTJB->mPos[2],
		glm_vec3_distance(pJB->mPos, pTJB->mPos));

	//see if both are already in a circuit
	if(pJB->mCircuit != -1 && pTJB->mCircuit != -1)
	{
		//in the same circuit already?
		if(pJB->mCircuit == pTJB->mCircuit)
		{
			//hoox
			pJB->mConnectedTo	=pTJB->mID;

			//the page says "nothing happens"
			//does this mean this shouldn't count as a connexion?
			sTotalConnexions++;
			return	true;
		}

		//merge circuits to lowest index
		if(pJB->mCircuit < pTJB->mCircuit)
		{
			sMergeCircuits(pJB->mCircuit, pTJB->mCircuit);
		}
		else
		{
			sMergeCircuits(pTJB->mCircuit, pJB->mCircuit);
		}

		pJB->mConnectedTo	=pTJB->mID;
		sTotalConnexions++;
		return	true;
	}

	//see if one is in a circuit already
	if(pJB->mCircuit != -1)
	{
		//add nearest to circuit
		sAddToCircuit(pJB->mCircuit, pTJB);
	}
	else if(pTJB->mCircuit != -1)
	{
		//add pJB to nearest's circuit
		sAddToCircuit(pTJB->mCircuit, pJB);
	}
	else
	{
		//add to an empty circuit
		int	freeCircuit	=sFindEmptyCircuit();

		assert(freeCircuit >= 0);

		sAddToCircuit(freeCircuit, pJB);
		sAddToCircuit(freeCircuit, pTJB);
	}
	pJB->mConnectedTo	=pTJB->mID;
	sTotalConnexions++;
	return	true;
}

static void	sGrabVec(const char *pLine, JunxBox *pJB)
{
	float	x	=atof(pLine);

	char	*pComma	=strchr(pLine, ',');

	float	y	=atof(pComma + 1);

	pComma	=strchr(pComma + 1, ',');

	float	z	=atof(pComma + 1);

	pJB->mPos[0]	=x;
	pJB->mPos[1]	=y;
	pJB->mPos[2]	=z;

	//init stuff while here
	pJB->mCircuit	=-1;
	pJB->next		=NULL;
	pJB->prev		=NULL;
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

static Material	*sMakeCubeMat(TestStuff *pTS, const StuffKeeper *pSK)
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
			CLAY_TEXT(CLAY_STRING("Visualizing playground..."),
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