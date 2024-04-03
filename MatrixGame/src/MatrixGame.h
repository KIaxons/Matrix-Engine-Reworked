// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once
#include "resource.h"
#include <shlobj.h>

class CMatrixMapLogic;
class CIFaceList;
class CRenderPipeline;
namespace Base {
	class CBlockPar;
	class CHeap;
	class CPoint;
}
class CLoadProgress;
class CHistory;
struct SMenuItemText;

namespace Base {
	extern CHeap* g_MatrixHeap;
	extern CBlockPar* g_MatrixData;
}
extern CMatrixMapLogic* g_MatrixMap;
extern CIFaceList*      g_IFaceList;
extern CRenderPipeline* g_Render;
extern CLoadProgress*   g_LoadProgress;
extern SMenuItemText*   g_PopupHead;
extern SMenuItemText*   g_PopupWeapon;
extern SMenuItemText*   g_PopupHull;
extern SMenuItemText*   g_PopupChassis;
extern CHistory*        g_ConfigHistory;

struct SRobotsSettings;

void MatrixGameInit(HINSTANCE hInstance, HWND wnd, wchar* map = nullptr, SRobotsSettings* set = nullptr, wchar* lang = nullptr, wchar* txt_start = nullptr, wchar* txt_win = nullptr, wchar* txt_loss = nullptr, wchar* planet = nullptr);
void MatrixGameDeinit();
LPCSTR PathToOutputFiles(LPCSTR dest);