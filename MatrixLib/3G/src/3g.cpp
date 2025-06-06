// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "stdafx.h"
#include "Texture.hpp"
#include "3g.hpp"
#include "Helper.hpp"
#include "../../../MatrixGame/src/MatrixSampleStateManager.hpp"

#include <stdio.h>

HINSTANCE g_HInst = 0;
IDirect3D9* g_D3D = nullptr;
IDirect3DDevice9* g_D3DD = nullptr;
D3DCAPS9 g_D3DDCaps;
ATOM g_WndA = 0;
HWND g_Wnd = 0;
bool g_WndExtern = false;
dword g_WndOldProg = 0;
CWStr* g_WndClassName = nullptr;
int g_ScreenX = 0, g_ScreenY = 0;
D3DPRESENT_PARAMETERS g_D3Dpp; 
//CReminder *g_Reminder;

#define SMOOTH_COUNT 16

dword g_Flags = 0;

float g_CamFieldOfView = 60.0f;
int g_DrawFPS = 0;
double g_DrawFPSMax_Period = (1000.0 / 50000.0);
int g_MaxFPS = 1000;
float g_MaxViewDistance = 4000.0f;
int g_MaxObjectsPerScreen = 2560;
int g_MaxEffectsCount = 1280;
int g_ShadowsDrawDistance = 1024;
float g_ThinFogDrawDistance = 0.5f;
float g_DenseFogDrawDistance = 0.7f;
int g_DrawFPSCur = 0;
int g_DrawFPSTime = 0;

int   g_PureGameTact = 0;       //������������ g_GameSpeedFactor "������" �������� ������� ����
float g_GameSpeedFactor = 1.0f; //����������� �������� ���� (������ �� �����)

int g_TactTime = 0;
int g_AvailableTexMem;

byte g_PlayerRobotsAutoBoom = 0;
bool g_EnableCheats = false;
bool g_EnableFlyers = false;
bool g_SelectableTurrets = false;

#ifdef DO_SMART_COLOROPS

STextureStageOp     g_ColorOp[8] = {{D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}, {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}, {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}, {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT},
                                    {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}, {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}, {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}, {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}};
STextureStageOp     g_AlphaOp[8] = {{D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}, {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}, {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}, {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT},
                                    {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}, {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}, {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}, {D3DTOP_DISABLE, D3DTA_CURRENT, D3DTA_CURRENT}};
#endif

static D3DGAMMARAMP g_StoreRamp0;
static D3DGAMMARAMP g_StoreRamp1;

LRESULT CALLBACK L3G_WndProc(HWND, UINT, WPARAM, LPARAM);

#ifdef _DEBUG
#include "D3DControl.hpp"

int  D3DResource::m_IB_cnt;
int  D3DResource::m_VB_cnt;
int  D3DResource::m_TEX_cnt;
dword D3DResource::m_UID_current;
D3DResource *D3DResource::m_First;
D3DResource *D3DResource::m_Last;
#include <stdio.h>

void D3DResource::StaticInit(void)
{
    m_IB_cnt = 0;
    m_VB_cnt = 0;
    m_TEX_cnt = 0;
    m_UID_current = 0;
    m_First = nullptr;
    m_Last = nullptr;
}
void D3DResource::Dump(D3DResType t)
{
    char buf[65536];
    strcpy(buf, "D3D Dump\n");
    D3DResource* el = m_First;
    while(el)
    {
        if(t == el->m_Type)
        {
            int l = strlen(buf);
            if(l < 65000)
            {
                sprintf(buf + l, "%u : %s - %i\n",el->m_UID, el->m_File, el->m_Line);
            }

        }

        el = el->m_Next;
    }

    SFT(buf);
    //MessageBox(g_Wnd, buf, "D3D Dump", MB_ICONINFORMATION);

    CBuf b(g_CacheHeap);
    b.Len(strlen(buf));
    memcpy(b.Get(), &buf, strlen(buf));
    b.SaveInFile(L"debug_dump.txt");
}
#endif

CWStr CExceptionD3D::Info()
{
    return CException::Info() + L"Text: {" + CWStr((wchar*)DXGetErrorStringW(m_Error)) + L"} " + CWStr((wchar*)DXGetErrorDescriptionW(m_Error));
}

void L3GInitAsEXE(HINSTANCE hinst, CBlockPar& bpcfg, const wchar_t* sysname, const wchar_t* captionname)
{
    RECT tr;

	L3GDeinit();

    g_DrawFPS = 0;
    g_DrawFPSMax_Period = (1000.0 / 50000.0);
    g_DrawFPSCur = 0;
    g_DrawFPSTime = 0;
    g_TactTime = 0;

    g_HInst = hinst;
    g_Wnd = nullptr;
    g_WndExtern = false;

	int cntpar = bpcfg.Par(L"FullScreen").GetCountPar(L",");

    CWStr str(bpcfg.Par(L"Resolution"), g_CacheHeap);
    if(str.GetCountPar(L",") < 2) ERROR_S(L"L3GInitAsEXE Error 1!");
    g_ScreenX = str.GetIntPar(0, L",");
    if(g_ScreenX <= 0) g_ScreenX = 1;
    g_ScreenY = str.GetIntPar(1, L",");
    if(g_ScreenY <= 0) g_ScreenY = 1;

    if(cntpar < 1) SETFLAG(g_Flags, GFLAG_FULLSCREEN);
	else INITFLAG(g_Flags, GFLAG_FULLSCREEN, bpcfg.Par(L"FullScreen").GetIntPar(0, L",") == 1);
  
    int bpp;
    if(cntpar < 2) bpp = 32;
    else
    {
        bpp = bpcfg.Par(L"FullScreen").GetIntPar(1, L",");
        if(bpp != 16 && bpp != 32) bpp = 32;
    }

    int refresh;

    if(cntpar < 3) refresh = 0;
    else refresh = bpcfg.Par(L"FullScreen").GetIntPar(2, L",");

    g_WndClassName = HNew(g_CacheHeap) CWStr(sysname, g_CacheHeap);
    *g_WndClassName += L"_wc";
	CStr classname(*g_WndClassName);

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc	= (WNDPROC)L3G_WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= g_HInst;
	wcex.hIcon			= nullptr;
	wcex.hCursor		= LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground	= 0;//(HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName	= nullptr;
	wcex.lpszClassName	= classname.Get();
	wcex.hIconSm		= nullptr;
    if(!(g_WndA = RegisterClassEx(&wcex))) ERROR_S(L"L3GInitAsEXE Error 2!");

    tr.left = 0;
    tr.top = 0;
    tr.right = g_ScreenX;
    tr.bottom = g_ScreenY;

    if(!FLAG(g_Flags, GFLAG_FULLSCREEN))
    {
        AdjustWindowRectEx(&tr, WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU, false, 0);
        g_Wnd = CreateWindow(classname.Get(), CStr(CWStr(captionname)).Get(), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER,
            0, 0, tr.right - tr.left, tr.bottom - tr.top, nullptr, nullptr, g_HInst, nullptr);
    }
    else
    {
        g_Wnd = CreateWindow(classname.Get(), CStr(CWStr(captionname)).Get(), WS_POPUP,
            0, 0, tr.right - tr.left, tr.bottom - tr.top, nullptr, nullptr, g_HInst, nullptr);
    }
    if(!g_Wnd) ERROR_S(L"L3GInitAsEXE Error 3!");

    GetClientRect(g_Wnd, &tr);
    if((g_ScreenX != (tr.right - tr.left)) || (g_ScreenY != (tr.bottom - tr.top))) ERROR_S(L"L3GInitAsEXE Resolution Error!");

    SetWindowLong(g_Wnd, GWL_WNDPROC, dword((WNDPROC)L3G_WndProc));

    IDirect3DSurface9* surf;
    g_D3DD->GetRenderTarget(0, &surf);
    if(!(surf == nullptr)) g_D3DD->ColorFill(surf, nullptr, 0);
    surf->Release();

    dword interval = D3DPRESENT_INTERVAL_IMMEDIATE;

    ZeroMemory(&g_D3Dpp, sizeof(g_D3Dpp));
    D3DDISPLAYMODE d3ddm;

	if(!FLAG(g_Flags, GFLAG_FULLSCREEN))
    {
	    ASSERT_DX(g_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm));
        if(d3ddm.Format == D3DFMT_X8R8G8B8)
        {
            d3ddm.Format = D3DFMT_A8R8G8B8;
            bpp = 32;
        }
        else
        {
            bpp = 16;
        }

		g_D3Dpp.Windowed = TRUE;
		//g_D3Dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		g_D3Dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
		g_D3Dpp.BackBufferFormat = d3ddm.Format;
		g_D3Dpp.EnableAutoDepthStencil = TRUE;
		g_D3Dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
        //d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
		g_D3Dpp.PresentationInterval = interval; // vsync off
	    g_D3Dpp.BackBufferCount = 2; // triple buffer
				
        g_D3Dpp.BackBufferWidth  = g_ScreenX;
        g_D3Dpp.BackBufferHeight = g_ScreenY;
	}
    else
    {
		g_D3Dpp.Windowed = FALSE;
		//g_D3Dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		g_D3Dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
        g_D3Dpp.BackBufferFormat = (bpp == 16) ? D3DFMT_R5G6B5 : D3DFMT_A8R8G8B8;
		g_D3Dpp.EnableAutoDepthStencil = TRUE;
		g_D3Dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
		g_D3Dpp.PresentationInterval = interval; // vsync off

        g_D3Dpp.FullScreen_RefreshRateInHz = refresh;

        g_D3Dpp.BackBufferWidth  = g_ScreenX;
        g_D3Dpp.BackBufferHeight = g_ScreenY;
	}
	
    g_D3Dpp.EnableAutoDepthStencil = TRUE;
    g_D3Dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
    g_D3Dpp.hDeviceWindow = g_Wnd;

    // Set default settings
    UINT AdapterToUse = D3DADAPTER_DEFAULT;
    D3DDEVTYPE DeviceType = D3DDEVTYPE_HAL;
	

    SETFLAG(g_Flags, GFLAG_STENCILAVAILABLE);

	ASSERT_DX(g_D3DD->GetDeviceCaps(&g_D3DDCaps));

    INITFLAG(g_Flags, GFLAG_GAMMA, g_D3DDCaps.Caps2&D3DCAPS2_CANCALIBRATEGAMMA);

    if (FLAG(g_Flags, GFLAG_GAMMA)) g_D3DD->GetGammaRamp(0, &g_StoreRamp0);

    g_AvailableTexMem = g_D3DD->GetAvailableTextureMem() / (1024 * 1024);

#ifdef DO_SMART_COLOROPS
    for(int i = 0; i < 8; ++i)
    {
		ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_COLOROP,		D3DTOP_DISABLE ));
        ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_COLORARG1,		D3DTA_CURRENT ));
        ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_COLORARG2,		D3DTA_CURRENT ));
		ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_ALPHAOP,		D3DTOP_DISABLE ));
        ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_ALPHAARG1,		D3DTA_CURRENT ));
        ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_ALPHAARG2,		D3DTA_CURRENT ));
    }
#endif
}

void L3GInitAsDLL(HINSTANCE hinst, CBlockPar& bpcfg, const wchar_t* sysname, const wchar_t* captionname, HWND hwnd, long FDirect3D, long FD3DDevice)
{
    //RECT tr;

    L3GDeinit();

    g_DrawFPS = 0;
    g_DrawFPSMax_Period = (1000.0 / 50000.0);
    g_DrawFPSCur = 0;
    g_DrawFPSTime = 0;
    g_TactTime = 0;

    g_HInst = hinst;
    g_Wnd = hwnd;
    g_WndExtern = true;

    g_D3D = (IDirect3D9*)FDirect3D;
    g_D3DD = (IDirect3DDevice9*)FD3DDevice;

    g_WndOldProg = GetWindowLong(g_Wnd, GWL_WNDPROC);
    if(g_WndOldProg == 0) ERROR_S(L"L3GInitAsDLL Error 1!");
    if(SetWindowLong(g_Wnd, GWL_WNDPROC, dword((WNDPROC)L3G_WndProc)) == 0) ERROR_S(L"L3GInitAsDLL Error 2!");

    /*IDirect3DSurface9 * surf;
    g_D3DD->GetRenderTarget(0,&surf);
    if (!(surf==nullptr)) g_D3DD->ColorFill(surf, nullptr, 0);
    surf->Release();*/

    ASSERT_DX(g_D3DD->GetDeviceCaps(&g_D3DDCaps));

    INITFLAG(g_Flags, GFLAG_GAMMA, g_D3DDCaps.Caps2 & D3DCAPS2_CANCALIBRATEGAMMA);

    if(FLAG(g_Flags, GFLAG_GAMMA)) g_D3DD->GetGammaRamp(0, &g_StoreRamp0);

    g_AvailableTexMem = g_D3DD->GetAvailableTextureMem() / (1024 * 1024);

#ifdef DO_SMART_COLOROPS
    for(int i = 0; i < 8; ++i)
    {
		ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_COLOROP,    D3DTOP_DISABLE));
        ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_COLORARG1,  D3DTA_CURRENT));
        ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_COLORARG2,  D3DTA_CURRENT));
		ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_ALPHAOP,    D3DTOP_DISABLE));
        ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_ALPHAARG1,  D3DTA_CURRENT));
        ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_ALPHAARG2,  D3DTA_CURRENT));
    }
#endif
}


void L3GDeinit()
{
    if(FLAG(g_Flags, GFLAG_GAMMA))
    {
        g_D3DD->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &g_StoreRamp0);
        g_D3DD->SetGammaRamp(1, D3DSGR_NO_CALIBRATION, &g_StoreRamp1);
    }

#if (defined _DEBUG) && !(defined _RELDEBUG)
    CHelper::ClearAll();
#endif

    /*
	if(g_D3DD)
    {
        int ref = g_D3DD->Release();
        g_D3DD = nullptr;
    }
	if(g_D3D)
    {
        int ref = g_D3D->Release();
        g_D3D = nullptr;
    }
    */
	ZeroMemory(&g_D3DDCaps, sizeof(D3DCAPS9));

	if(g_WndA)
    {
        if(g_Wnd)
        {
            DestroyWindow(g_Wnd);
            g_Wnd = 0;
        }

        UnregisterClass(CStr(*g_WndClassName).Get(), g_HInst);
        g_WndA = 0;

        HDelete(CWStr, g_WndClassName, g_CacheHeap);
	}

    if(g_WndExtern) SetWindowLong(g_Wnd, GWL_WNDPROC, g_WndOldProg);

    g_WndExtern = false;
}

int L3GRun()
{
	MSG msg; //ZeroMemory(&msg, sizeof(MSG));

    double freq_inv;
    union
    {
        unsigned __int64 freq;
        LARGE_INTEGER freq_li;
    };
    union
    {
        unsigned __int64 tact;
        LARGE_INTEGER takt_li;
    };
    union
    {
        unsigned __int64 draw_time_last;
        LARGE_INTEGER draw_time_last_li;
    };

    unsigned __int64 zero_offset;

    QueryPerformanceFrequency(&freq_li);
    freq_inv = 1000.0 / double(freq);

    QueryPerformanceCounter(&takt_li);
    draw_time_last = tact;
    zero_offset = tact;

    float prev_takt = 10.0f;

    int smooth[SMOOTH_COUNT];
    int smooths = SMOOTH_COUNT * 10;
    int smp = 0;
    for(int i = 0; i < SMOOTH_COUNT; ++i) smooth[i] = 10;

	while(true)
    {
		while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
			//TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if(msg.message == WM_QUIT) break;
		if(FLAG(g_Flags, GFLAG_APPCLOSE)) break;
        if(FLAG(g_Flags, GFLAG_EXITLOOP)) break;
		if(FLAG(g_Flags, GFLAG_APPACTIVE))
        {
            union
            {
                __int64 ctakt;
                LARGE_INTEGER ctakt_li;
            };

            QueryPerformanceCounter(&ctakt_li);
            ctakt -= zero_offset;
            float cur_takt = float(double(ctakt - tact) * freq_inv);
            if(cur_takt <= 0) cur_takt = prev_takt;
            prev_takt = cur_takt;

			if(g_FormCur)
            {
                float tt1 = min(100, cur_takt);
                int tt = Float2Int(tt1);

                smooths -= smooth[smp];
                smooths += tt;
                smooth[smp] = tt;
                smp = (smp + 1) & (SMOOTH_COUNT - 1);

                g_PureGameTact = tt;
                tt = smooths / int(SMOOTH_COUNT * g_GameSpeedFactor);

                SRemindCore::Tact(tt);
				g_FormCur->Tact(tt); //������� �������� ���������� ����������� ����
			}

            tact = ctakt;

            if((double(ctakt - draw_time_last) * freq_inv) > g_DrawFPSMax_Period)
            {
				draw_time_last = ctakt;
				++g_DrawFPSCur;

                if(g_FormCur)
                {
					g_FormCur->Draw();

#if (defined _DEBUG) && !(defined _RELDEBUG)
                    CHelper::AfterDraw();
#endif
				}
			}
            else ctakt = draw_time_last;

            int cms = Double2Int(double(ctakt) * freq_inv);
            if((cms - g_DrawFPSTime) > 1000)
            {
                g_DrawFPSTime = cms;
                g_DrawFPS = g_DrawFPSCur;
                g_DrawFPSCur = 0;

                g_AvailableTexMem = g_D3DD->GetAvailableTextureMem() / (1024 * 1024);
                //DM(L"MatrixGame.FPS", CWStr(g_DrawFPS).Get());
			}

            //Sleep(10);
		}
        else Sleep(1);
	}

	return 1;
}

LRESULT CALLBACK L3G_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
    {
		case WM_PAINT:
        {
            //if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->Draw();
            // do not draw by paint message
            break;
        }
        /*
        case WM_ACTIVATE:
        {
            if((LOWORD(wParam) == WA_ACTIVE) || (LOWORD(wParam) == WA_CLICKACTIVE))
            {
                if(HIWORD(wParam) == 0)
                {
                    SETFLAG(g_Flags, GFLAG_APPACTIVE);
                    if(FLAG(g_Flags, GFLAG_FULLSCREEN)) ShowWindow(g_Wnd, SW_MAXIMIZE);
                    else ShowWindow(g_Wnd, SW_RESTORE);
                    if(g_FormCur) g_FormCur->SystemEvent(SYSEV_ACTIVATED);
                    //g_D3DD->TestCooperativeLevel();
                    //CRect rc;
                    //GetClientRect(hWnd, &rc);
                    //ClientToScreen(hWnd, (CPoint*)&rc.left);
                    //reen(hWnd, (CPoint*)&rc.right);
                    //ClipCursor(&rc);
                }
            }
            else
            {
                //ClipCursor(nullptr);
                if(g_FormCur) g_FormCur->SystemEvent(SYSEV_DEACTIVATING);
                RESETFLAG(g_Flags, GFLAG_APPACTIVE);

                //SetWindowTextA(g_Wnd, "Click me");
                ShowWindow(g_Wnd, SW_MINIMIZE);
                ClipCursor(nullptr);
                g_D3DD->TestCooperativeLevel();
            }
            break;
        }
        */
		//case WM_ACTIVATE:
		case WM_ACTIVATEAPP:
        {
            //g_D3DD->TestCooperativeLevel();
            if(wParam != 0)
            {
                SETFLAG(g_Flags, GFLAG_APPACTIVE);
                if(FLAG(g_Flags, GFLAG_FULLSCREEN))
                {
                    g_D3Dpp.Windowed = FALSE;
                    g_D3DD->Reset(&g_D3Dpp);
                    ShowWindow(g_Wnd, SW_MAXIMIZE);
                }
                else ShowWindow(g_Wnd, SW_RESTORE);
                g_D3DD->TestCooperativeLevel();
                //if(g_D3DD->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) g_D3DD->Reset(&g_D3Dpp);
                if(g_FormCur) g_FormCur->SystemEvent(SYSEV_ACTIVATED);
            }
            else
            {
                RESETFLAG(g_Flags, GFLAG_APPACTIVE);
                if(g_FormCur) g_FormCur->SystemEvent(SYSEV_DEACTIVATING);

                if(FLAG(g_Flags, GFLAG_FULLSCREEN))
                {
                    g_D3Dpp.Windowed = TRUE;
                    g_D3DD->Reset(&g_D3Dpp);
                }
                ShowWindow(g_Wnd, SW_MINIMIZE);
                ClipCursor(nullptr);
                g_D3DD->TestCooperativeLevel();
            }
            break;
        }
        case 0x020A: //WM_MOUSEWHEEL: // */) {(msg == 0x020A/*
        {
//#define GET_KEYSTATE_WPARAM(wParam)     (LOWORD(wParam))
#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))

            //int fwKeys = GET_KEYSTATE_WPARAM(wParam);
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / 120;
            if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->MouseKey(B_WHEEL, zDelta, short(LOWORD(lParam)), short(HIWORD(lParam)));
            return 0;
        }
		case WM_MOUSEMOVE:
        {
            if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->MouseMove(short(LOWORD(lParam)), short(HIWORD(lParam)));
            return 0;
        }
		case WM_LBUTTONDOWN:
        {
            if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->MouseKey(B_DOWN/*true*/, VK_LBUTTON, short(LOWORD(lParam)), short(HIWORD(lParam)));
            return 0;
        }
        case WM_LBUTTONDBLCLK:
        {
            /*
            POINT p;
            if(GetCursorPos(&p))
            {
                if(ScreenToClient(g_Wnd, &p))
                {
                    if(p.y >= g_ScreenY * 0.9)
                    {
                        if(g_FormCur) g_FormCur->SystemEvent(SYSEV_DEACTIVATING);
                        RESETFLAG(g_Flags, GFLAG_APPACTIVE);
                        ShowWindow(g_Wnd, SW_MINIMIZE);
                        ClipCursor(nullptr);
                        g_D3DD->TestCooperativeLevel();
                        break;
                    }
                }
            }
            */

            if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->MouseKey(B_DOUBLE/*true*/, VK_LBUTTON, short(LOWORD(lParam)), short(HIWORD(lParam)));
            return 0;
        }
		case WM_RBUTTONDOWN:
        {
            if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->MouseKey(B_DOWN/*true*/, VK_RBUTTON, short(LOWORD(lParam)), short(HIWORD(lParam)));
            return 0;
        }
        case WM_RBUTTONDBLCLK:
        {
            if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->MouseKey(B_DOUBLE/*true*/, VK_RBUTTON, short(LOWORD(lParam)), short(HIWORD(lParam)));
            return 0;
        }
        case WM_MBUTTONDOWN:
        {
            if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->MouseKey(B_DOWN/*true*/, VK_MBUTTON, short(LOWORD(lParam)), short(HIWORD(lParam)));
            return 0;
        }
		case WM_LBUTTONUP:
        {
            if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->MouseKey(B_UP/*false*/, VK_LBUTTON, short(LOWORD(lParam)), short(HIWORD(lParam)));
            return 0;
        }
		case WM_RBUTTONUP:
        {
            if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->MouseKey(B_UP/*false*/, VK_RBUTTON, short(LOWORD(lParam)), short(HIWORD(lParam)));
            return 0;
        }
		case WM_MBUTTONUP:
        {
			if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->MouseKey(B_UP/*false*/, VK_MBUTTON, short(LOWORD(lParam)), short(HIWORD(lParam)));
			return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->Keyboard(true, lp2key(lParam));
            break;
        }
        //������ ��� ����� ������� ������ PrintScreen
        case WM_HOTKEY:
        {
            if(GetAsyncKeyState(VK_SNAPSHOT)) g_FormCur->Keyboard(true, KEY_SNAPSHOT);
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            if(FLAG(g_Flags, GFLAG_APPACTIVE) && g_FormCur) g_FormCur->Keyboard(false, lp2key(lParam));
            break;
        }
        case WM_SETCURSOR:
        {
            //g_D3DD->ShowCursor(true);
            return true; //Prevent Windows from setting cursor to window class cursor
        }
		case WM_DESTROY:
        case WM_CLOSE:
        {
            SETFLAG(g_Flags, GFLAG_APPCLOSE);
            //PostQuitMessage(0);
            return 0;
        }
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void S3D_Default()
{
	D3DXMATRIX mWorld;
	D3DXMatrixIdentity(&mWorld);
	ASSERT_DX(g_D3DD->SetTransform( D3DTS_WORLD, &mWorld ));

	float fBias=-1.0f;

	for(int i=0;i<8;i++) {
		ASSERT_DX(g_Sampler.SetState(i,D3DSAMP_MAGFILTER,			D3DTEXF_LINEAR));
		ASSERT_DX(g_Sampler.SetState(i,D3DSAMP_MINFILTER,			D3DTEXF_LINEAR));

		ASSERT_DX(g_D3DD->SetSamplerState(i,D3DSAMP_ADDRESSU,			D3DTADDRESS_WRAP));
		ASSERT_DX(g_D3DD->SetSamplerState(i,D3DSAMP_ADDRESSV,			D3DTADDRESS_WRAP));

		ASSERT_DX(g_Sampler.SetState(i,D3DSAMP_MIPFILTER,			D3DTEXF_LINEAR));
		ASSERT_DX(g_D3DD->SetSamplerState(i,D3DSAMP_MIPMAPLODBIAS,		*((LPDWORD) (&fBias))));

	    ASSERT_DX(g_D3DD->SetTexture( i, nullptr ));
		ASSERT_DX(g_D3DD->SetTextureStageState(i,D3DTSS_TEXCOORDINDEX,	0 ));
	}

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_SHADEMODE,			D3DSHADE_GOURAUD ));

	//ASSERT_DX(g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR,		0xffffffff ));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE,	FALSE));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_SRCBLEND,			D3DBLEND_SRCALPHA ));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_DESTBLEND,			D3DBLEND_INVSRCALPHA ));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE,		FALSE));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAREF,			0x08 ));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAFUNC,			D3DCMP_GREATEREQUAL ));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE,			D3DCULL_CCW));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_LIGHTING,			FALSE));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZENABLE,				D3DZB_TRUE));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE,		TRUE));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZFUNC,				D3DCMP_LESSEQUAL));
    //ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZFUNC,				D3DCMP_LESS));

}



