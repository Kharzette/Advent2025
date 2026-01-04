#define NOB_IMPLEMENTATION
#define	NOB_EXPERIMENTAL_DELETE_OLD
#include	"../GrogLibsC/nob.h"
#include	<stdlib.h>


//static forward decs
static bool	sBuildLib(const char *szPath, bool bCpp, bool bReBuild);
static bool	sBuildCLib(const char *szPath);
static bool	sBuildCppLib(const char *szPath);
static bool	sBuildProgram(const char *szDir, const char *szFile, bool bReBuild);
static int	sNeedsBuild(const char *szPath, const char *szFile, bool bCpp);
static bool	sbIsC(const char *szFileName);
static bool	sbIsCpp(const char *szFileName);
static void	sStandardLibJunk_Add(Nob_Cmd *pCmd);


//possible command line args:
//--dayxx-x
int	main(int argc, char **argv)
{
	//rebuild this program if need be
	NOB_GO_REBUILD_URSELF(argc, argv);

	//check command line args here?
	bool	bDay08p1	=false;
	bool	bDay08p2	=false;
	bool	bDay09p2	=false;
	if(argc > 1)
	{
		for(int i=0;i < argc;i++)
		{
			if(strncmp("--day08-1", argv[i], 9) == 0)
			{
				bDay08p1	=true;
			}
			else if(strncmp("--day08-2", argv[i], 9) == 0)
			{
				bDay08p2	=true;
			}
			if(strncmp("--day09-2", argv[i], 9) == 0)
			{
				bDay09p2	=true;
			}
		}
	}

	if(bDay08p1)
	{
		if(!sBuildProgram("Day08", "playground", false))
		{
			return	EXIT_FAILURE;
		}
	}
	else if(bDay08p2)
	{
		if(!sBuildProgram("Day08", "playgroundPart2", false))
		{
			return	EXIT_FAILURE;
		}
	}
	else if(bDay09p2)
	{
		if(!sBuildProgram("Day09", "theaterBSP", false))
		{
			return	EXIT_FAILURE;
		}
	}


	return	EXIT_SUCCESS;
}


//statics
static void	sStandardLibJunk_Add(Nob_Cmd *pCmd)
{
	//stuff all the libraries like
	//do I still need that linker crap for shared libs?
	nob_cmd_append(pCmd, "gcc", "-g", "-O0", "-march=native",
		"-fpic", "-shared",
		"-DCGLM_FORCE_DEPTH_ZERO_TO_ONE",
		"-DCGLM_FORCE_LEFT_HANDED",
		"-I../uthash/src",
		"-I../dxvk-native/include/native/directx",
		"-I../dxvk-native/include/native/windows",
		"-I../dxvk-native/include/native",
		"-I../dxvk-native/sse2neon",
		"-I../dxvk-native/include",
		"-I../cglm/include", "-Wall",
//		"-Wall", "-fsanitize=address",
		"-Wl,-rpath=$ORIGIN,--disable-new-dtags");
}

static void	sStandardProgJunk_Add(Nob_Cmd *pCmd)
{
	nob_cmd_append(pCmd, "gcc", "-g", "-O0", "-march=native",
		"-DCGLM_FORCE_DEPTH_ZERO_TO_ONE",
		"-DCGLM_FORCE_LEFT_HANDED",
		"-I../GrogLibsC/uthash/src",
		"-I../GrogLibsC/dxvk-native/include/native/directx",
		"-I../GrogLibsC/dxvk-native/include/native/windows",
		"-I../GrogLibsC/cglm/include",
		"-I../GrogLibsC",
		"-Wall", //"-fsanitize=address",
		"-Wl,-rpath=libs,--disable-new-dtags");
}

static bool	sbIsC(const char *szFileName)
{
	size_t	len	=strlen(szFileName);
	if(len < 3)
	{
		return	false;
	}

	return	(szFileName[len - 2] == '.'
		&& szFileName[len - 1] == 'c');
}

static bool	sbIsCpp(const char *szFileName)
{
	size_t	len	=strlen(szFileName);
	if(len < 5)
	{
		return	false;
	}

	return	(szFileName[len - 4] == '.'
		&& szFileName[len - 3] == 'c'
		&& szFileName[len - 2] == 'p'
		&& szFileName[len - 1] == 'p');
}

//returns < 0 if error, 0 if no build needed
static int sNeedsBuild(const char *szPath, const char *szFile, bool bCpp)
{
	//cd to dir
	if(!nob_set_current_dir(szPath))
	{
		printf("Something went wrong changing to lib dir %s\n", szPath);
		return	false;
	}

	//make the wierdly named so file name
	char	tmp[128];
	sprintf(tmp, "../libs/lib%s.so", szPath);

	//just one specified c file
    Nob_File_Paths	cFiles		={0};
	nob_da_append(&cFiles, szFile);

	int	buildNeeded	=nob_needs_rebuild(tmp, cFiles.items, cFiles.count);

	nob_da_free(cFiles);
	
	//pop back up a dir
	if(!nob_set_current_dir(".."))
	{
		printf("Something went wrong changing to ..\n");
		return	-1;
	}
	return	buildNeeded;
}

static bool sBuildCppLib(const char *szPath)
{
	Nob_Cmd	libCmd	={0};

	//cd to dir
	if(!nob_set_current_dir(szPath))
	{
		printf("Something went wrong changing to lib dir %s\n", szPath);
		return	false;
	}
	
	sStandardLibJunk_Add(&libCmd);

	//extra stuff needed by jolt things
	nob_cmd_append(&libCmd,
		"-std=gnu++23",
		"-DJPH_PROFILE_ENABLED",
		"-DJPH_DEBUG_RENDERER",
		"-DJPH_OBJECT_STREAM",
		"-I../JoltPhysics");

    Nob_File_Paths	cppFiles	={0};

	if(!nob_read_entire_dir(".", &cppFiles))
	{
		printf("Something went wrong reading dir %s\n", szPath);
		return	false;
	}

	//add all the .cpp files
	for(int i=0;i < cppFiles.count;i++)
	{
		if(sbIsCpp(cppFiles.items[i]))
		{
			nob_cmd_append(&libCmd, cppFiles.items[i]);
		}
	}

	//more args
	nob_cmd_append(&libCmd, "-o");

	//make the wierdly named so file name
	char	tmp[128];
	sprintf(tmp, "../libs/lib%s.so", szPath);
	nob_cmd_append(&libCmd,
		tmp, "../JoltPhysics/Build/Linux_Debug/libJolt.a");

	bool	bWorked	=nob_cmd_run(&libCmd);

	//pop back up a dir
	if(!nob_set_current_dir(".."))
	{
		printf("Something went wrong changing to ..\n");
		return	false;
	}

	nob_da_free(cppFiles);
	nob_cmd_free(libCmd);
	
	return	bWorked;
}

static bool	sBuildCLib(const char *szPath)
{
	Nob_Cmd	libCmd	={0};

	//cd to dir
	if(!nob_set_current_dir(szPath))
	{
		printf("Something went wrong changing to lib dir %s\n", szPath);
		return	false;
	}

	sStandardLibJunk_Add(&libCmd);

    Nob_File_Paths	cFiles		={0};

	if(!nob_read_entire_dir(".", &cFiles))
	{
		printf("Something went wrong reading dir %s\n", szPath);
		return	false;
	}

	//add all the .c files
	for(int i=0;i < cFiles.count;i++)
	{
		if(sbIsC(cFiles.items[i]))
		{
			nob_cmd_append(&libCmd, cFiles.items[i]);
		}
	}

	//hack for tinywav
	if(strcmp(szPath, "AudioLib") == 0)
	{
		nob_cmd_append(&libCmd, "tinywav/tinywav.c");
	}

	//more args
	nob_cmd_append(&libCmd, "-std=gnu23", "-o");

	//make the wierdly named so file name
	char	tmp[128];
	sprintf(tmp, "../libs/lib%s.so", szPath);

	nob_cmd_append(&libCmd,
		tmp, "-lvulkan", "-lpng16", "-lSDL3",
		"../dxvk-native/build/src/dxgi/libdxvk_dxgi.so",
		"../dxvk-native/build/src/d3d11/libdxvk_d3d11.so");

	bool	bWorked	=nob_cmd_run(&libCmd);

	//pop back up a dir
	if(!nob_set_current_dir(".."))
	{
		printf("Something went wrong changing to ..\n");
		return	false;
	}

	nob_da_free(cFiles);
	nob_cmd_free(libCmd);
	
	return	bWorked;
}

//pass in the dir and program name (TestUI etc...)
static bool	sBuildProgram(const char *szDir, const char *szFile, bool bReBuild)
{
	//cd to dir
	if(!nob_set_current_dir(szDir))
	{
		printf("Something went wrong changing to test dir\n");
		return	false;
	}

	//thing .c
	char	tmp[128];
	sprintf(tmp, "%s.c", szFile);
	
	if(!bReBuild)
	{
		int	needsBuild	=nob_needs_rebuild1(szFile, tmp);
		if(needsBuild < 0)
		{
			nob_set_current_dir("..");
			return	false;	//error
		}
		else if(!needsBuild)
		{
			printf("No build needed for %s...\n", szFile);
			nob_set_current_dir("..");
			return	true;	//no build needed
		}
	}

	Nob_Cmd	progCmd	={0};

	sStandardProgJunk_Add(&progCmd);

	//add .c file
	nob_cmd_append(&progCmd, tmp);

	//more args
	nob_cmd_append(&progCmd, "-std=gnu23", "-o");

	nob_cmd_append(&progCmd, szFile,
		"-L../GrogLibsC/libs",				//link with stuff in ../GrogLibsC/libs
		"-lm", "-lvulkan", "-lUtilityLib", "-lPhysicsLib",
		"-lMaterialLib", "-lUILib", "-lMeshLib",
		"-lTerrainLib", "-lInputLib", "-lAudioLib",
		"-lFAudio", "-lSDL3", "-lpng",
		"../GrogLibsC/dxvk-native/build/src/dxgi/libdxvk_dxgi.so",
		"../GrogLibsC/dxvk-native/build/src/d3d11/libdxvk_d3d11.so");

	bool	bWorked	=nob_cmd_run(&progCmd);

	//pop back up a dir
	if(!nob_set_current_dir(".."))
	{
		printf("Something went wrong changing to ..\n");
		return	false;
	}

	nob_cmd_free(progCmd);
	
	return	bWorked;
}