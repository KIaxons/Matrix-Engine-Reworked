// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

#include "basedef.hpp"

inline void    memcopy_back_dword(void *tgt, const void *src, dword size) //same as blk_copy, but copying by sizeof(uint) bytes
{
    dword    *uitgt = ((dword *)tgt) + size - 1;
    const dword    *uisrc = ((const dword *)src) + size - 1;
    while(size)
    {
        *uitgt-- =  *uisrc--;
        size--;
    }
}


