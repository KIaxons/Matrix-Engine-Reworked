// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

enum ESupportError {
    SUPE_OK             = 0,
    SUPE_DIRECTX        = 1, // non 9.x DirectX
    SUPE_VIDEODRIVER    = 2, // driver too old
    SUPE_VIDEOHARDWARE  = 3, // video hardware unsupported
};

enum EExitState {
    ES_EXIT_TO_WINDOWS = 0,
    ES_EXIT_TO_MAIN_MENU,
    ES_EXIT_AFTER_WIN,
    ES_EXIT_AFTER_LOSE,
    ES_EXIT_AFTER_SURRENDER
};

struct SRobotsSettings
{   
	long  FDirect3D;
    long  FD3DDevice;
    // можно менять во время игры
    bool  m_ShowStencilShadows;
    bool  m_ShowProjShadows;
    bool  m_IzvratMS;                // извратный selection объектов

    // нужен рестарт
    bool  m_LandTexturesGloss;
    bool  m_ObjTexturesGloss;
    bool  m_SoftwareCursor;
    byte  m_SkyBox;                  // 0 - none, 1 - dds (low quality), 2 - png (high quality)

    int   m_BPP;                     // 16, 32
    int   m_ResolutionX;
    int   m_ResolutionY;
    int   m_RefreshRate;             // 0 - default

    float m_Brightness;
    float m_Contrast;

	int   m_FSAASamples;
	int   m_AFDegree;
	float m_MaxDistance;
	bool  m_VSync;

    void ApplyVideoParams();
};

struct SRobotGameState
{
    int m_Time;
    int m_BuildRobot;
    int m_KillRobot;
    int m_BuildTurret;
    int m_KillTurret;
    int m_KillBuilding;
};

struct SMGDRobotInterface;
struct SMGDRangersInterface;
struct SMGDRangersInterfaceText;

struct SMGDRobotInterface
{
    void(__stdcall*   m_Init)(SMGDRangersInterface* ri);
    void(__stdcall*   m_Deinit)();
    int(__stdcall*    m_Support)();
    int(__stdcall*    m_Run)(HINSTANCE hinst, HWND hwnd, wchar* map, SRobotsSettings* set, wchar* lang, wchar* txt_start, wchar* txt_win, wchar* txt_loss, wchar* planet, SRobotGameState* rgs);
};

struct SMGDRangersInterface
{
    void(__stdcall*   m_Sound)(wchar* path);

    dword(__stdcall*  m_SoundCreate)(wchar* path, int group, int loop);
    void(__stdcall*   m_SoundDestroy)(dword id);
    void(__stdcall*   m_SoundPlay)(dword id);
    int(__stdcall*    m_SoundIsPlay)(dword id);
    void(__stdcall*   m_SoundVolume)(dword id, float vol);
    void(__stdcall*   m_SoundPan)(dword id, float pan);
    float(__stdcall*  m_SoundGetVolume)(dword id);
    float(__stdcall*  m_SoundGetPan)(dword id);

    void(__stdcall*   m_RangersText)(wchar* text, wchar* font, dword color, int sizex, int sizey, int alignx, int aligny, int wordwrap, int smex, int smy, CRect* clipr, SMGDRangersInterfaceText* it);
    void(__stdcall*   m_RangersTextClear)(SMGDRangersInterfaceText* it);

    void(__stdcall*   m_ProgressBar)(float val);

    void(__stdcall*   m_Tact)(void);

    void(__stdcall*   m_Begin)(void);

    float(__stdcall*  m_MusicVolumeGet)(void);
    void(__stdcall*   m_MusicVolumeSet)(float val);
};

struct SMGDRangersInterfaceText
{
    dword m_Image;
    byte* m_Buf;
    dword m_Pitch;
    int   m_SizeX;
    int   m_SizeY;
};

extern "C"
{
    MATRIXGAMEDLL_API SMGDRobotInterface* __cdecl GetRobotInterface(void);
}

extern SMGDRobotInterface g_RobotInterface;
extern SMGDRangersInterface* g_RangersInterface;

// 0-exit to windows (terminate)
// 1-exit to main menu
// 2-loss
// 3-win
// 4-cancel
extern int g_ExitState;