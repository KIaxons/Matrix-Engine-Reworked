// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#ifndef MATRIX_LOAD_PROGRESS_INCLUDE
#define MATRIX_LOAD_PROGRESS_INCLUDE

#define LPACCURACY 100

#define  LP_PRELOADROBOTS     0
#define  LP_LOADINGMAP        1
#define  LP_PREPARINGOBJECTS  2
#define  LP_PREPARININTERFACE 3

struct SLoadProcessProps
{
    const wchar* description = L"";
    int len = 0;
};

static SLoadProcessProps lp_props[] =
{
    { L"Preloading robots", 70 },
    { L"Loading map", 300 },
    { L"Preparing objects", 150 },
    { L"Preparing Interface", 30 },
};

class CLoadProgress : public CMain
{
    int m_CurLoadProcess = -1;
    int m_fullsize = 0;
    float m_fullsize1 = 0.0f;
    int m_cursizedone = 0;
    int m_cur_lp_size = 0;
    float m_cur_lp_size1 = 0.0f;

    int m_lastacc = 0;

public:
    CLoadProgress()
    {
        const int n = sizeof(lp_props) / sizeof(SLoadProcessProps);
        for(int i = 0; i < n; ++i) m_fullsize += lp_props[i].len;
        m_fullsize1 = 1.0f / float(m_fullsize);
    }
    ~CLoadProgress() = default;

    void SetCurLP(int lp);
    void InitCurLP(int sz) { m_cur_lp_size = sz; m_cur_lp_size1 = 1.0f / float(sz); };
    void SetCurLPPos(int i);
};

#endif