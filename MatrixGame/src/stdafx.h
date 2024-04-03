// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef STDAFX_MAIN_HEADER
#define STDAFX_MAIN_HEADER

// Windows Header Files:
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h> // _alloca (всегда выделяет память со стека, кидает эксепшен при нехватке), _malloca (выделяет память со стека только до _ALLOCA_S_THRESHOLD, а далее только из общей кучи, так что требует ручной очистки после вызова через _freea(), кроме того НЕ кидает эксепшен, тупо возвращая nullptr)
#include <memory.h>
#include <tchar.h>

// TODO: reference additional headers your program requires here
#include "../../MatrixLib/3G/src/stdafx.h"

#include "../../MatrixLib/3G/include/3g.hpp"
#include "../../MatrixLib/3G/include/VectorObject.hpp"
#include "../../MatrixLib/3G/include/CBillboard.hpp"
#include "../../MatrixLib/3G/include/BigVB.hpp"
#include "../../MatrixLib/3G/include/BigIB.hpp"
#include "../../MatrixLib/DebugMsg/include/DebugMsg.h"

#include "MatrixMultiSelection.hpp"

#ifdef MATRIXGAME_EXPORTS
#define MATRIXGAMEDLL_API __declspec(dllexport)
#else
#define MATRIXGAMEDLL_API __declspec(dllimport)
#endif

#include "MatrixGame.h"

using namespace Base;

#endif