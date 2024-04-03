// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

// Windows Header Files:
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h> // _alloca (всегда выделяет память со стека, кидает эксепшен при нехватке), _malloca (выделяет память со стека только до _ALLOCA_S_THRESHOLD, а далее только из общей кучи, так что требует ручной очистки после вызова через _freea(), кроме того НЕ кидает эксепшен, тупо возвращая nullptr)
#include <memory.h>
#include <tchar.h>
#include <Mmsystem.h>

// TODO: reference additional headers your program requires here
#include "d3d9.h"
#include "d3dx9tex.h"
//#include "dxerr9.h"
#include "dxerr.h"

#include "../../Bitmap/src/stdafx.h"
#include "../../Base/include/base.hpp"

#include "../../Bitmap/include/CBitmap.hpp"
#include "../../DebugMsg/include/DebugMsg.h"


using namespace Base;

#include "3g.hpp"
