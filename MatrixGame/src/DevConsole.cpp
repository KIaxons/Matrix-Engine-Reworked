// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "DevConsole.hpp"

static void hHelp(const Base::CWStr& cmd, const Base::CWStr& params)
{
    g_MatrixMap->m_Console.ShowHelp();
}

static void hShadows(const Base::CWStr& cmd, const Base::CWStr& params)
{
    if(params.GetLen() == 2)
    {
        g_Config.m_ShowStencilShadows = params[0] == '1';
        g_Config.m_ShowProjShadows = params[1] == '1';
    }

    g_MatrixMap->m_DI.ShowScreenText(L"Stencil shadows", g_Config.m_ShowStencilShadows ? L"ON" : L"OFF");
    g_MatrixMap->m_DI.ShowScreenText(L"Proj shadows", g_Config.m_ShowProjShadows ? L"ON" : L"OFF");
}

static void hCannon(const Base::CWStr& cmd, const Base::CWStr& params)
{
    if (params.GetLen() == 1)
    {
        g_Config.m_CannonsLogic = params[0] == '1';
    }

    g_MatrixMap->m_DI.ShowScreenText(L"Cannon's logic", g_Config.m_CannonsLogic ? L"ON" : L"OFF");
}

static void hLog(const Base::CWStr& cmd, const Base::CWStr& params)
{
    if(params == L"s")
    {
        CSound::SaveSoundLog();
    }
    else if(params == L"e")
    {
        CBuf b(g_CacheHeap);
        b.StrNZ("Effects:\n");

        for(int i = 0; i < EFFECT_TYPE_COUNT; ++i)
        {
            int cnt = 0;
            for(CMatrixEffect* e = g_MatrixMap->m_EffectsFirst; e != nullptr; e = e->m_Next)
            {
                if(e->GetType() == (EEffectType)i)
                {
                    ++cnt;
                }
            }

            if(cnt == 0) continue;
            CStr ss(g_CacheHeap);
            ss = i;
            ss += " - ";
            ss += cnt;
            ss += "\n";

            b.StrNZ(ss);
        }

        b.SaveInFile(L"log.txt");
    }
}

static void hBuildCFG(const Base::CWStr& cmd, const Base::CWStr& params)
{
    CBlockPar bpi(1, g_CacheHeap);
    bpi.LoadFromTextFile(IF_PATH);

    CStorage stor(g_CacheHeap);

    CBlockPar data(1, g_CacheHeap);
    data.CopyFrom(*g_MatrixData);
    data.BlockDelete(PAR_REPLACE);

    stor.StoreBlockPar(L"if", bpi);
    stor.StoreBlockPar(L"da", data);

    stor.Save(FILE_CONFIGURATION, true);
}

static void hTestSpdTrace(const Base::CWStr& cmd, const Base::CWStr& params)
{
    srand(1);
    D3DXVECTOR3 pos1, pos2;

    dword time1 = timeGetTime();

    for(int i = 0; i < 100000; ++i)
    {

        pos1.x = FRND(g_MatrixMap->m_Size.x * GLOBAL_SCALE);
        pos1.y = FRND(g_MatrixMap->m_Size.x * GLOBAL_SCALE);
        pos1.z = FRND(20) + 10;
        pos2.x = FRND(g_MatrixMap->m_Size.x * GLOBAL_SCALE);
        pos2.y = FRND(g_MatrixMap->m_Size.x * GLOBAL_SCALE);
        pos2.z = FRND(20) + 10;

        pos1.z += g_MatrixMap->GetZ(pos1.x, pos1.y);
        pos2.z += g_MatrixMap->GetZ(pos2.x, pos2.y);


        g_MatrixMap->Trace(nullptr, pos1, pos2, TRACE_LANDSCAPE, nullptr);
    }

    dword time2 = timeGetTime();

    g_MatrixMap->m_DI.ShowScreenText(L"Trace time (ms)", CWStr(time2 - time1, g_CacheHeap), 5000);
}

static void hMusic(const Base::CWStr& cmd, const Base::CWStr& params)
{
    if(params == L"1") g_MatrixMap->RestoreMusicVolume();
    else if(params == L"0") g_MatrixMap->SetMusicVolume(0);
}

static void hCalcVis(const Base::CWStr& cmd, const Base::CWStr& params)
{
    g_MatrixMap->CalcVis();
}

static void hCompress(const Base::CWStr& cmd, const Base::CWStr& params)
{
    CWStr name(g_CacheHeap);
    if(CFile::FileExist(name, params))
    {
        CBuf fil(g_CacheHeap);
        CStorage out(g_CacheHeap);
        fil.LoadFromFile(params);

        CStorageRecord sr(CWStr(L"0", g_CacheHeap), g_CacheHeap);
        sr.AddItem(CStorageRecordItem(CWStr(L"0", g_CacheHeap), ST_BYTE));
        out.AddRecord(sr);

        CDataBuf* b = out.GetBuf(L"0", L"0", ST_BYTE);
        b->AddArray();
        b->AddToArray<byte>(0, (byte*)fil.Get(), fil.Len());

        CacheReplaceFileExt(name, params, L".strg");

        out.Save(name, true);
    }
}

SCmdItem CDevConsole::m_Commands[] =
{
    { L"HELP", hHelp },
    { L"SHADOWS", hShadows },
    { L"CANNON", hCannon },
    { L"LOG", hLog },
    { L"TRACESPD", hTestSpdTrace },
    { L"BUILDCFG", hBuildCFG },
    { L"MUSIC", hMusic },
    { L"COMPRESS", hCompress },
    { L"CALCVIS", hCalcVis },

    { nullptr, nullptr } // last
};

static wchar Scan2Char(int scan)
{
    switch(scan)
    {
        case KEY_SPACE: return ' ';
        case KEY_A: return 'A';
        case KEY_B: return 'B';
        case KEY_C: return 'C';
        case KEY_D: return 'D';
        case KEY_E: return 'E';
        case KEY_F: return 'F';
        case KEY_G: return 'G';
        case KEY_H: return 'H';
        case KEY_I: return 'I';
        case KEY_J: return 'J';
        case KEY_K: return 'K';
        case KEY_L: return 'L';
        case KEY_M: return 'M';
        case KEY_N: return 'N';
        case KEY_O: return 'O';
        case KEY_P: return 'P';
        case KEY_Q: return 'Q';
        case KEY_R: return 'R';
        case KEY_S: return 'S';
        case KEY_T: return 'T';
        case KEY_U: return 'U';
        case KEY_V: return 'V';
        case KEY_W: return 'W';
        case KEY_X: return 'X';
        case KEY_Y: return 'Y';
        case KEY_Z: return 'Z';
        case KEY_0: return '0';
        case KEY_1: return '1';
        case KEY_2: return '2';
        case KEY_3: return '3';
        case KEY_4: return '4';
        case KEY_5: return '5';
        case KEY_6: return '6';
        case KEY_7: return '7';
        case KEY_8: return '8';
        case KEY_9: return '9';
        case KEY_LSLASH: return '\\';
        case KEY_COMMA: return '.';

        default: return 0;
    }
}

bool CDevConsole::IsActive() const
{
    return FLAG(m_Flags, DCON_ACTIVE);
}

void CDevConsole::SetActive(bool set)
{
    INITFLAG(m_Flags, DCON_ACTIVE, set);
    if(set) m_NextTime = g_MatrixMap->GetTime();
}

void CDevConsole::ShowHelp(void)
{
    int i = 0;
    while(m_Commands[i].cmd != nullptr)
    {
        CWStr desc(g_MatrixHeap);

        switch(i)
        {
            case 0: desc = L"Shows help"; break;
            case 1: desc = L"Switch shadows : [0|1][0|1]"; break;
            case 2: desc = L"Switch cannons logic : [0|1]"; break;
        }

        g_MatrixMap->m_DI.ShowScreenText(m_Commands[i].cmd, desc, 5000);
        ++i;
    }
}

void CDevConsole::Tact(int ms)
{
    m_Time += ms;
    while(m_NextTime < m_Time)
    {
        m_NextTime += DEV_CONSOLE_CURSOR_FLASH_PERIOD;
        INVERTFLAG(m_Flags, DCON_CURSOR);
    }

    CWStr out(g_MatrixHeap);
    out.Set(m_Text.Get(), m_CurPos);

    if(FLAG(m_Flags, DCON_CURSOR))
    {
        //out.Add(L"&");
        out.Add(L"|");
    }

    if(m_CurPos < m_Text.GetLen())
    {
        out.Add(m_Text.Get() + m_CurPos);
    }
    else
    {
        out.Add(L" ");
    }

    g_MatrixMap->m_DI.ShowScreenText(L"Console", out.Get(), 1000);
}

void CDevConsole::Keyboard(int scan, bool down)
{
    if(down)
    {
        SETFLAG(m_Flags, DCON_CURSOR);
        if(scan == KEY_BACKSPACE)
        {
            if(m_CurPos > 0)
            {
                --m_CurPos;
                m_Text.Del(m_CurPos, 1);
            }
        }
        else if(scan == KEY_DELETE)
        {
            if(m_CurPos < m_Text.GetLen())
            {
                m_Text.Del(m_CurPos, 1);
            }
        }
        else if(scan == KEY_LEFT)
        {
            if(m_CurPos > 0)
            {
                --m_CurPos;
            }
        }
        else if(scan == KEY_RIGHT)
        {
            if(m_CurPos < m_Text.GetLen())
            {
                ++m_CurPos;
            }
        }
        else if(scan == KEY_HOME)
        {
            m_CurPos = 0;
        }
        else if(scan == KEY_END)
        {
            m_CurPos = m_Text.GetLen();
        }
        else if(scan == KEY_ENTER)
        {
            CWStr cmd(g_MatrixHeap);
            CWStr params(g_MatrixHeap);
            int i = 0;
            while(i < m_Text.GetLen())
            {
                if(m_Text[i] == ' ') break;
                ++i;
            }

            cmd.Set(m_Text.Get(), i);
            if(i < m_Text.GetLen()) params.Set(m_Text.Get() + i + 1);
            cmd.UpperCase();

            i = 0;
            while(m_Commands[i].cmd != nullptr)
            {
                if(m_Commands[i].cmd == cmd)
                {
                    m_Commands[i].handler(cmd, params);
                    m_Text.SetLen(0);
                    m_CurPos = 0;
                    break;
                }
                ++i;
            }
        }
        else if(scan == KEY_ESC)
        {
            if(m_Text.GetLen() == 0) SetActive(false);
            else
            {
                m_Text.SetLen(0);
                m_CurPos = 0;
            }
        }
        else if(scan == KEY_LSHIFT || scan == KEY_RSHIFT)
        {
            SETFLAG(m_Flags, DCON_SHIFT);
        }
        else
        {
            wchar c = Scan2Char(scan);
            if(c != 0)
            {
                if(!FLAG(m_Flags, DCON_SHIFT))
                {
                    if(c >= 'A' && c <= 'Z') c |= 32;
                }
                m_Text.Insert(m_CurPos, CWStr(c, g_MatrixHeap));
                ++m_CurPos;
            }
        }
    }
    else
    {
        if(scan == KEY_LSHIFT || scan == KEY_RSHIFT)
        {
            RESETFLAG(m_Flags, DCON_SHIFT);
        }
    }
}