// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "stdafx.h"
#include "MatrixLoadProgress.hpp"
#include "MatrixGameDll.hpp"


void CLoadProgress::SetCurLP(int lp)
{
    if(m_CurLoadProcess >= 0)
    {
        m_cursizedone += lp_props[m_CurLoadProcess].len;
    }

    m_CurLoadProcess = lp;

#ifdef _DEBUG
    OutputDebugStringW(L"\n" + CWStr(lp_props[lp].description));
#endif
}

void CLoadProgress::SetCurLPPos(int i)
{
    float k = m_fullsize1 * (m_cursizedone + ((m_cur_lp_size1 * float(i)) * lp_props[m_CurLoadProcess].len));

    int ac = Float2Int(k * LPACCURACY);
    if(ac > m_lastacc)
    {
        if(g_RangersInterface) g_RangersInterface->m_HealthBar(float(ac) / LPACCURACY);

#ifdef _DEBUG
        OutputDebugStringW(L"\n" + CWStr(ac));
#endif

        m_lastacc = ac;
    }
}