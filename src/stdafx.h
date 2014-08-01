#pragma once

#include <PlayrixEngine.h>
#include <Utils/Xml.h>

#ifdef _DEBUG
	#ifdef ENGINE_TARGET_WIN32
		#define MyAssert(e) if(!(e)){DebugBreak();}
	#else
		#define MyAssert(e) Assert(e)
	#endif
#else
	#define MyAssert(e)
#endif


#pragma warning( once : 4996 ) // deprecated functions
#pragma warning( once : 331 )
