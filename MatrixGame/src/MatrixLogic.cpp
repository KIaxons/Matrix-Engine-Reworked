// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "stdafx.h"
#include "MatrixLogic.hpp"
#include "MatrixObject.hpp"
#include "MatrixObjectBuilding.hpp"
#include "MatrixRobot.hpp"
#include "Interface/CInterface.h"
#include "MatrixFlyer.hpp"
#include "MatrixObjectCannon.hpp"
#include "Interface/MatrixHint.hpp"
#include <stdio.h>
#include "MatrixGameDll.hpp"

//CPoint MatrixDir45[8] = {CPoint(-1, 0), CPoint(1, 0), CPoint(0,-1), CPoint(0, 1),
//						CPoint(-1, -1), CPoint(1, 1), CPoint(-1, 1), CPoint(1, -1)};

CMatrixRobotAI *g_TestRobot = nullptr;
bool g_TestLocal = false;

CMatrixMapLogic::~CMatrixMapLogic()
{
    Clear();
}

void CMatrixMapLogic::Clear()
{
    LeaveDialogMode();
    PathClear();
    ObjClear();

    m_MPFCnt = m_MPF2Cnt = 0;
    if(m_MPF)
    {
        HFree(m_MPF, Base::g_MatrixHeap);
        m_MPF = nullptr;
    }

    if(m_MPF2)
    {
        HFree(m_MPF2, Base::g_MatrixHeap);
        m_MPF2 = nullptr;
    }

    if(m_ZoneIndex)
    {
        HFree(m_ZoneIndex, Base::g_MatrixHeap);
        m_ZoneIndex = nullptr;
    }

    if(m_ZoneIndexAccess)
    {
        HFree(m_ZoneIndexAccess, Base::g_MatrixHeap);
        m_ZoneIndexAccess = nullptr;
    }

    if(m_ZoneDataZero)
    {
        HFree(m_ZoneDataZero, Base::g_MatrixHeap);
        m_ZoneDataZero = nullptr;
    }

    if(m_MapPoint)
    {
        HFree(m_MapPoint, Base::g_MatrixHeap); m_MapPoint = nullptr;
    }

    //ZoneClear();
    CMatrixMap::Clear();
}

int CMatrixMapLogic::Rnd()
{
    m_Rnd = 16807 * (m_Rnd % 127773) - 2836 * (m_Rnd / 127773);
    if(m_Rnd <= 0) m_Rnd = m_Rnd + 2147483647;
    return m_Rnd - 1;
}

float CMatrixMapLogic::RndFloat()
{
    return float(Rnd()) / 2147483647.0f - 2.0f;
}

int CMatrixMapLogic::Rnd(int min, int max)
{
    if(min <= max) return min + (Rnd() % (max - min + 1));
    else return max + (Rnd() % (min - max + 1));
}

float CMatrixMapLogic::RndFloat(float min, float max)
{
    return min + RndFloat() * (max - min);
}

void CMatrixMapLogic::GatherInfo(int type)
{
    CMatrixMapStatic* obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive())
        {
            obj->AsRobot()->GatherInfo(type);
        }

        obj = obj->GetNextLogic();
    }
}

void CMatrixMapLogic::PrepareBuf()
{
#if (defined _DEBUG) && !(defined _RELDEBUG)
    if(m_ZoneDataZero)
    {
        int cnt = max(m_RoadNetwork.m_ZoneCnt, m_RoadNetwork.m_PlaceCnt);
        dword* buf = m_ZoneDataZero;
        while(cnt)
        {
            if(*buf != 0) ERROR_S(L"CMatrixMapLogic::PrepareBuf Error!");
            ++buf;
            --cnt;
        }
    }
#endif

    if(!m_ZoneIndex) m_ZoneIndex = (int*)HAlloc(max(m_RoadNetwork.m_ZoneCnt, m_RoadNetwork.m_PlaceCnt) * sizeof(int), Base::g_MatrixHeap);
    if(!m_ZoneDataZero) m_ZoneDataZero = (dword*)HAllocClear(max(m_RoadNetwork.m_ZoneCnt, m_RoadNetwork.m_PlaceCnt) * sizeof(dword), Base::g_MatrixHeap);
    if(!m_ZoneIndexAccess) m_ZoneIndexAccess = (int*)HAlloc(max(m_RoadNetwork.m_ZoneCnt, m_RoadNetwork.m_PlaceCnt) * sizeof(int), Base::g_MatrixHeap);
}

//void CMatrixMapLogic::ZoneClear()
//{
//    DTRACE();
//
//	for(int i=0;i<5;i++) {
//		if(m_Zone[i]!=nullptr) {
//			HFree(m_Zone[i],g_MatrixHeap);
//			m_Zone[i]=nullptr;
//		}
//		m_ZoneCnt[i]=0;
//	}
//}
//
//void CMatrixMapLogic::ZoneCalc(int nsh,byte mm)
//{
//    DTRACE();
//
//	int x,y,cx,cy,i,u,t,k;
//	SMatrixMapUnit * mu;
//	CMatrixMapStatic * ms;
//
//	// Clear zone
//	mu=UnitGet(0,0);
//	for(y=0;y<m_Size.y;y++) {
//		for(x=0;x<m_Size.x;x++,mu++) {
//			mu->m_Zone[nsh]=-1;
//		}
//	}
//
//	// Find begin zone
//	int zonesize=16;
//
//	SMatrixMapZone * zone=(SMatrixMapZone *)HAllocClear(sizeof(SMatrixMapZone)*(m_Size.x/zonesize+1)*(m_Size.y/zonesize+1),g_MatrixHeap);
//	int zonecnt=0;
//
//	for(y=zonesize/2;y<m_Size.y;y+=zonesize) {
//		for(x=zonesize/2;x<m_Size.x;x+=zonesize) {
//			zone[zonecnt].m_Building=false;
//			zone[zonecnt].m_BeginX=x;
//			zone[zonecnt].m_BeginY=y;
//			ms=m_StaticFirst;
//			while(ms)
//            {
//				if(ms->GetObjectType() == OBJECT_TYPE_BUILDING)
//                {
//                    cx=Float2Int(((CMatrixBuilding *)ms)->m_Pos.x / GLOBAL_SCALE_MOVE);
//					cy=Float2Int(((CMatrixBuilding *)ms)->m_Pos.y / GLOBAL_SCALE_MOVE);
//					if(cx>=(x-zonesize/2) && cx<(x+zonesize/2) && cy>=(y-zonesize/2) && cy<(y+zonesize/2)) {
//						zone[zonecnt].m_BeginX=cx;
//						zone[zonecnt].m_BeginY=cy;
//						zone[zonecnt].m_Building=true;
//						break;
//					}
//				}
//				ms=ms->m_Next;
//			}
//			if(zone[zonecnt].m_Building) { zonecnt++; continue; }
//			mu=UnitGet(zone[zonecnt].m_BeginX,zone[zonecnt].m_BeginY);
//			if(!(mu->m_Stop && mm)) { zonecnt++; continue; }
//
//			bool fo=false;
//			for(i=0;i<(zonesize/2-1);i++) {
//				for(u=0;u<((i+1)*2+1);u++) {
//					mu=UnitGetTest(x-(i+1)+u,y-(i+1));
//					if(mu && !(mu->m_Stop & mm)) {
//						zone[zonecnt].m_BeginX=x-(i+1)+u;
//						zone[zonecnt].m_BeginY=y-(i+1);
//						fo=true; 
//						break; 
//					}
//					mu=UnitGetTest(x-(i+1)+u,y+(i+1));
//					if(mu && !(mu->m_Stop & mm)) {
//						zone[zonecnt].m_BeginX=x-(i+1)+u;
//						zone[zonecnt].m_BeginY=y+(i+1);
//						fo=true; 
//						break; 
//					}
//				}
//				if(fo) break;
//
//				for(u=0;u<(i*2+1);u++) {
//					mu=UnitGetTest(x-(i+1),y-i+u);
//					if(mu && !(mu->m_Stop & mm)) {
//						zone[zonecnt].m_BeginX=x-(i+1);
//						zone[zonecnt].m_BeginY=y-i+u;
//						fo=true; 
//						break; 
//					}
//					mu=UnitGetTest(x+(i+1),y-i+u);
//					if(mu && !(mu->m_Stop & mm)) {
//						zone[zonecnt].m_BeginX=x+(i+1);
//						zone[zonecnt].m_BeginY=y-i+u;
//						fo=true; 
//						break; 
//					}
//				}
//				if(fo) break;
//			}
//			if(fo) zonecnt++;
//		}
//	}
//
//	for(i=0;i<zonecnt;i++) {
//		mu=UnitGet(zone[i].m_BeginX,zone[i].m_BeginY);
//		mu->m_Zone[nsh]=i;
//	}
//
//	// Grow zone, prepate center, calc rect, calc size
//	if(zonecnt>0) {
//		struct sgz {
//			int cnt,cnt2;
//			int max;
//			CPoint * p,* p2;
//		} * pgz=(sgz *)HAllocClear(sizeof(sgz)*zonecnt,g_MatrixHeap);
//
//		for(i=0;i<zonecnt;i++) {
//			pgz[i].max=zonesize*8;
//			pgz[i].p=(CPoint *)HAlloc(sizeof(CPoint)*pgz[i].max,g_MatrixHeap);
//			pgz[i].p2=(CPoint *)HAlloc(sizeof(CPoint)*pgz[i].max,g_MatrixHeap);
//			pgz[i].p[0].x=zone[i].m_BeginX;
//			pgz[i].p[0].y=zone[i].m_BeginY;
//			pgz[i].cnt=1;
//
//			zone[i].m_Rect.left=1000000;
//			zone[i].m_Rect.top=1000000;
//			zone[i].m_Rect.right=-1000000;
//			zone[i].m_Rect.bottom=-1000000;
//		}
//
//		bool growok=true;
//		while(growok) {
//			growok=false;
//
//			for(i=0;i<zonecnt;i++) {
//				if(pgz[i].cnt+4>pgz[i].max) {
//					pgz[i].max+=32;
//					pgz[i].p=(CPoint *)HAllocEx(pgz[i].p,sizeof(CPoint)*pgz[i].max,g_MatrixHeap);
//					pgz[i].p2=(CPoint *)HAllocEx(pgz[i].p2,sizeof(CPoint)*pgz[i].max,g_MatrixHeap);
//				}
//				for(u=0;u<pgz[i].cnt;u++) {
//					zone[i].m_CenterX+=pgz[i].p[u].x;
//					zone[i].m_CenterY+=pgz[i].p[u].y;
//
//					if(pgz[i].p[u].x<zone[i].m_Rect.left) zone[i].m_Rect.left=pgz[i].p[u].x;
//					if(pgz[i].p[u].x>=zone[i].m_Rect.right) zone[i].m_Rect.right=pgz[i].p[u].x+1;
//					if(pgz[i].p[u].y<zone[i].m_Rect.top) zone[i].m_Rect.top=pgz[i].p[u].y;
//					if(pgz[i].p[u].y>=zone[i].m_Rect.bottom) zone[i].m_Rect.bottom=pgz[i].p[u].y+1;
//
//					zone[i].m_Size++;
//				}
//
//				for(u=0;u<pgz[i].cnt;u++) {
//					CPoint cp=pgz[i].p[u];
//					mu=UnitGetTest(cp.x-1,cp.y); if(mu && mu->m_Zone[nsh]<0 && !(mu->m_Stop & mm)) { mu->m_Zone[nsh]=i; pgz[i].p2[pgz[i].cnt2].x=cp.x-1; pgz[i].p2[pgz[i].cnt2].y=cp.y; pgz[i].cnt2++; growok=true; }
//					mu=UnitGetTest(cp.x+1,cp.y); if(mu && mu->m_Zone[nsh]<0 && !(mu->m_Stop & mm)) { mu->m_Zone[nsh]=i; pgz[i].p2[pgz[i].cnt2].x=cp.x+1; pgz[i].p2[pgz[i].cnt2].y=cp.y; pgz[i].cnt2++; growok=true; }
//					mu=UnitGetTest(cp.x,cp.y-1); if(mu && mu->m_Zone[nsh]<0 && !(mu->m_Stop & mm)) { mu->m_Zone[nsh]=i; pgz[i].p2[pgz[i].cnt2].x=cp.x; pgz[i].p2[pgz[i].cnt2].y=cp.y-1; pgz[i].cnt2++; growok=true; }
//					mu=UnitGetTest(cp.x,cp.y+1); if(mu && mu->m_Zone[nsh]<0 && !(mu->m_Stop & mm)) { mu->m_Zone[nsh]=i; pgz[i].p2[pgz[i].cnt2].x=cp.x; pgz[i].p2[pgz[i].cnt2].y=cp.y+1; pgz[i].cnt2++; growok=true; }
//				}
//
//				CPoint * pt=pgz[i].p; pgz[i].p=pgz[i].p2; pgz[i].p2=pt;
//				pgz[i].cnt=pgz[i].cnt2; pgz[i].cnt2=0;
//			}
//		}
//
//		for(i=0;i<zonecnt;i++) {
//			HFree(pgz[i].p,g_MatrixHeap);
//			HFree(pgz[i].p2,g_MatrixHeap);
//		}
//		HFree(pgz,g_MatrixHeap);
//	}
//
//	// find center, calc perimeter, find near zone
//	for(i=0;i<zonecnt;i++) {
//		zone[i].m_CenterX/=zone[i].m_Size;
//		zone[i].m_CenterY/=zone[i].m_Size;
//
////		if(UnitGet(zone[i].m_CenterX,zone[i].m_CenterY)->zone!=i) {
//			cx=zone[i].m_BeginX;
//			cy=zone[i].m_BeginY;
//			u=1000000000;
//			for(y=zone[i].m_Rect.top;y<zone[i].m_Rect.bottom;y++) {
//				for(x=zone[i].m_Rect.left;x<zone[i].m_Rect.right;x++) {
//					if(UnitGet(x,y)->m_Zone[nsh]==i) {
//						t=(x-zone[i].m_CenterX)*(x-zone[i].m_CenterX)+(y-zone[i].m_CenterY)*(y-zone[i].m_CenterY);
//						if(t<u) {
//							cx=x; cy=y;
//							u=t;
//						}
//
//						for(u=0;u<4;u++) {
//							if(u==0) mu=UnitGetTest(x-1,y);
//							else if(u==1) mu=UnitGetTest(x+1,y);
//							else if(u==2) mu=UnitGetTest(x,y-1);
//							else if(u==3) mu=UnitGetTest(x,y+1);
//
//							if(!mu) continue;
//
//							if(mu->m_Zone[nsh]!=i) {
//								zone[i].m_Perim++;
//
//								if(mu->m_Zone[nsh]>=0) {
//									for(k=0;k<zone[i].m_NearZoneCnt;k++) if(zone[i].m_NearZone[k]==mu->m_Zone[nsh]) break;
//									if(k>=zone[i].m_NearZoneCnt) {
//										ASSERT(zone[i].m_NearZoneCnt+1<=8);
//										zone[i].m_NearZone[zone[i].m_NearZoneCnt]=mu->m_Zone[nsh];
//										zone[i].m_NearZoneConnectSize[zone[i].m_NearZoneCnt]=1;
//										zone[i].m_NearZoneCnt++;
//									} else {
//										zone[i].m_NearZoneConnectSize[k]++;
//									}
//								}
//							}
//						}
//					}
//				}
//			}
//			zone[i].m_CenterX=cx;
//			zone[i].m_CenterY=cy;
////		}
//	}
//
//	// Delete connect m_NearZoneConnectSize<1
//	for(i=0;i<zonecnt;i++) {
//		for(k=0;k<zone[i].m_NearZoneCnt;k++) {
//			if(zone[i].m_NearZoneConnectSize[k]<1) {
//				MoveMemory(zone[i].m_NearZone+k,zone[i].m_NearZone+k+1,(zone[i].m_NearZoneCnt-k-1)*sizeof(int));
//				MoveMemory(zone[i].m_NearZoneConnectSize+k,zone[i].m_NearZoneConnectSize+k+1,(zone[i].m_NearZoneCnt-k-1)*sizeof(int));
//				zone[i].m_NearZoneCnt--;
//			}
//		}
//	}
//
//	// Delete zone if NearCnt==0
//	for(i=0;i<zonecnt;i++) {
//		if(zone[i].m_NearZoneCnt>0) continue;
//
//		mu=UnitGet(0,0);
//		for(y=0;y<m_Size.y;y++) {
//			for(x=0;x<m_Size.x;x++,mu++) {
//				if(mu->m_Zone[nsh]==i) { 
//					mu->m_Zone[nsh]=-1;// mu->m_Move|=(1+2+4+8+16); 
//				}
//				else if(mu->m_Zone[nsh]>i) mu->m_Zone[nsh]--;
//			}
//		}
//
//		for(u=0;u<zonecnt;u++) {
//			for(k=0;k<zone[u].m_NearZoneCnt;k++) {
//				if(zone[u].m_NearZone[k]>i) zone[u].m_NearZone[k]--;
//			}
//		}
//
//		if(zonecnt-i-1>0) MoveMemory(zone+i,zone+i+1,(zonecnt-i-1)*sizeof(SMatrixMapZone));
//		i--;
//		zonecnt--;
//	}
//
//	// Куда не можем попасть делаем непроходящими
////	mu=UnitGet(0,0);
////	for(y=0;y<m_Size.y;y++) {
////		for(x=0;x<m_Size.x;x++,mu++) {
////			if(mu->zone<0) mu->m_Move=1+2+4+8+16;
////		}
////	}
//
//	zone=(SMatrixMapZone *)HAllocEx(zone,sizeof(SMatrixMapZone)*zonecnt,g_MatrixHeap);
//
//	m_Zone[nsh]=zone;
//	m_ZoneCnt[nsh]=zonecnt;
//}
//
//void CMatrixMapLogic::ZoneCalc()
//{
//    DTRACE();
//
//	ZoneClear();
//
//	ZoneCalc(0,1);
//	ZoneCalc(1,2);
//	ZoneCalc(2,4);
//	ZoneCalc(3,8);
//	ZoneCalc(4,16);
//}

int CMatrixMapLogic::ZoneFindNear(int nsh, int mx, int my)
{
    DTRACE();

    int i;

    SMatrixMapMove* smm = MoveGetTest(mx, my);
    if(!smm)
    {
        ERROR_S(L"CMatrixMapLogic::ZoneFindNear Error 1!");
    }

    if(smm->m_Zone >= 0 && (nsh < 0 || !(smm->m_Stop & (1 << nsh)))) return smm->m_Zone;

    int mind = 1000000000;
    int minz = -1;
    for(i = 0; i < m_RoadNetwork.m_ZoneCnt; ++i)
    {
        int d = (mx - m_RoadNetwork.m_Zone[i].m_Center.x) * (mx - m_RoadNetwork.m_Zone[i].m_Center.x) + (my - m_RoadNetwork.m_Zone[i].m_Center.y) * (my - m_RoadNetwork.m_Zone[i].m_Center.y);
        if(d < mind)
        {
            mind = d;
            minz = i;
        }
    }

    if(minz < 0) ERROR_S(L"CMatrixMapLogic::ZoneFindNear Error 2!");

    return minz;
}

void CMatrixMapLogic::PlaceGet(int nsh, float wx, float wy, int* mx, int* my)
{
    DTRACE();

    int xx = Float2Int(wx * INVERT(GLOBAL_SCALE_MOVE));
    int yy = Float2Int(wy * INVERT(GLOBAL_SCALE_MOVE));

    if(xx < 0) xx = 0;
    else if(xx >= m_SizeMove.x) xx = m_SizeMove.x - 1;

    if(yy < 0) yy = 0;
    else if(yy >= m_SizeMove.y) yy = m_SizeMove.y - 1;

    *mx = xx;
    *my = yy;

    SMatrixMapMove* smm = MoveGetTest(*mx, *my);
    if(!smm) return;

    if(!(smm->m_Stop & (1 << nsh))) return;

    smm = MoveGetTest(*mx + 1, *my);
    if(smm && !(smm->m_Stop & (1 << nsh)))
    {
        ++(*mx);
        return;
    }

    smm = MoveGetTest(*mx, *my + 1);
    if(smm && !(smm->m_Stop & (1 << nsh)))
    {
        ++(*my);
        return;
    }

    smm = MoveGetTest(*mx + 1, *my + 1);
    if(smm && !(smm->m_Stop & (1 << nsh)))
    {
        ++(*mx);
        ++(*my);
        return;
    }

    //if(smm && smm->m_Zone >= 0) return;

    /*if((wx - float(*mx)) > 0.8)
    {
		smm = MoveGetTest(*mx + 1, *my);
		if(smm && smm->m_Zone >= 0)
        {
			++(*mx);
		}
	}
	if((wy - float(*my)) > 0.8)
    {
		smm = MoveGetTest(*mx, *my + 1);
		if(smm && smm->m_Zone >= 0)
        {
			++(*my);
		}
	}*/
}

bool CMatrixMapLogic::IsAbsenceWall(int nsh, int size, int mx, int my)
{
    DTRACE();

    if(mx < 0 || mx + size > m_SizeMove.x || my < 0 || my + size > m_SizeMove.y) return false;

    ASSERT(size >= 1 && size <= 5);
    dword nsh_mask = (1 << nsh) << (6 * (size - 1));

    SMatrixMapMove *smm = MoveGet(mx, my);
    return (smm->m_Stop & nsh_mask) == 0;

    /*SMatrixMapMove* smm = MoveGet(mx, my);
	for(int y = 0; y < size; ++y, smm += m_SizeMove.x-size)
    {
		for(int x = 0; x < size; ++x, ++smm)
        {
            if(nsh < 0)
            {
                if(smm->m_Stop) return false;
            }
            else
            {
                if(smm->m_Stop & (1 << nsh)) return false;
            }
		}
	}
	return true;*/
}



/*bool CMatrixMapLogic::PlaceFindNear(int nsh,int size,int & mx,int & my)
{
    DTRACE();

	if(PlaceIsEmpty(nsh,size,mx,my)) return true;

	byte mm=1<<nsh;

	SMatrixMapMove * smm=MoveGetTest(mx,my);

	if(smm && !(smm->m_Stop & mm) && size==2) {
		if(PlaceIsEmpty(nsh,size,mx,my-1)) { my--; return true; }
		else if(PlaceIsEmpty(nsh,size,mx-1,my)) { mx--; return true; }
		else if(PlaceIsEmpty(nsh,size,mx-1,my-1)) { mx--; my--; return true; }
	}

	int u,i=0;
	while(i<m_SizeMove.x) {
		for(u=0;u<((i+1)*2+1);u++) {
			if(PlaceIsEmpty(nsh,size,mx-(i+1)+u,my-(i+1))) { mx=mx-(i+1)+u; my=my-(i+1); return true; }
			if(PlaceIsEmpty(nsh,size,mx-(i+1)+u,my+(i+1))) { mx=mx-(i+1)+u; my=my+(i+1); return true; }
		}

		for(u=0;u<(i*2+1);u++) {
			if(PlaceIsEmpty(nsh,size,mx-(i+1),my-i+u)) { mx=mx-(i+1); my=my-i+u; return true; }
			if(PlaceIsEmpty(nsh,size,mx+(i+1),my-i+u)) { mx=mx+(i+1); my=my-i+u; return true; }
		}
		i++;
	}
	return false;
}*/

bool CMatrixMapLogic::PlaceFindNear(int nsh, int size, int& mx, int& my, int other_cnt, int* other_size, CPoint* other_des)
{
    int k, tx, ty;
    int *os;
    CPoint *od;

    if(IsAbsenceWall(nsh, size, mx, my))
    {
        for(k = 0, os = other_size, od = other_des; k < other_cnt; ++k, ++od, ++os)
        {
            if(!(od->x + *os <= mx || od->x >= mx + size) && !(od->y + *os <= my || od->y >= my + size)) break;
        }
        if(k >= other_cnt) return true;
    }

    //SMatrixMapMove *smm = MoveGetTest(mx, my);

    int u, i = 0;
    while(i < m_SizeMove.x)
    {
        for(u = 0; u < ((i + 1) * 2 + 1); ++u)
        {
            tx = mx - (i + 1) + u; ty = my - (i + 1);
            if(IsAbsenceWall(nsh, size, tx, ty))
            {
                for(k = 0, os = other_size, od = other_des; k < other_cnt; ++k, ++od, ++os)
                {
                    if(!(od->x + *os <= tx || od->x >= tx + size) && !(od->y + *os <= ty || od->y >= ty + size)) break;
                }
                if(k >= other_cnt) { mx = tx; my = ty; return true; }
            }
            tx = mx - (i + 1) + u;
            ty = my + (i + 1);

            if(IsAbsenceWall(nsh, size, tx, ty))
            {
                for(k = 0, os = other_size, od = other_des;k < other_cnt; ++k, ++od, ++os)
                {
                    if(!(od->x + *os <= tx || od->x >= tx + size) && !(od->y + *os <= ty || od->y >= ty + size)) break;
                }
                if(k >= other_cnt) { mx = tx; my = ty; return true; }
            }
        }

        for(u = 0; u < (i * 2 + 1); ++u)
        {
            tx = mx - (i + 1); ty = my - i + u;
            if(IsAbsenceWall(nsh, size, tx, ty))
            {
                for(k = 0, os = other_size, od = other_des; k < other_cnt; ++k, ++od, ++os)
                {
                    if(!(od->x + *os <= tx || od->x >= tx + size) && !(od->y + *os <= ty || od->y >= ty + size)) break;
                }
                if(k >= other_cnt)
                {
                    mx = tx;
                    my = ty;
                    return true;
                }
            }
            tx = mx + (i + 1);
            ty = my - i + u;

            if(IsAbsenceWall(nsh, size, tx, ty))
            {
                for(k = 0, os = other_size, od = other_des; k < other_cnt; ++k, ++od, ++os)
                {
                    if(!(od->x + *os <= tx || od->x >= tx + size) && !(od->y + *os <= ty || od->y >= ty + size)) break;
                }
                if(k >= other_cnt)
                {
                    mx = tx;
                    my = ty;
                    return true;
                }
            }
        }
        ++i;
    }
    return false;
}

//bool CMatrixMapLogic::PlaceFindNear(int nsh,int size,int & mx,int & my,const D3DXVECTOR2 & vdir,int other_cnt,int * other_size,CPoint * other_des)
//{
//    DTRACE();
//
//    int k,tx,ty;
//    int * os;
//    CPoint * od;
//
//    float sx=GLOBAL_SCALE_MOVE*mx+GLOBAL_SCALE_MOVE*size/2;
//    float sy=GLOBAL_SCALE_MOVE*my+GLOBAL_SCALE_MOVE*size/2;
//
//    float vx=vdir.x;
//    float vy=vdir.y;
//    float vd=1/sqrt(POW2(vx)+POW2(vy));
//    vx*=vd; vy*=vd;
//
//	SMatrixMapMove * smm=MoveGetTest(mx,my);
//
//    float pa=cos(80.0f*ToRad);
//
//	int u,i=0;
//	while(i<m_SizeMove.x) {
//		for(u=0;u<((i+1)*2+1);u++) {
//            tx=mx-(i+1)+u; ty=my-(i+1);
//			if(IsAbsenceWall(nsh,size,tx,ty)) { 
//                float dx=GLOBAL_SCALE_MOVE*tx+GLOBAL_SCALE_MOVE*size/2-sx;
//                float dy=GLOBAL_SCALE_MOVE*ty+GLOBAL_SCALE_MOVE*size/2-sy;
//                float t=1.0f/sqrt(POW2(dx)+POW2(dy));
//                dx*=t; dy*=t;
//                if(fabs(dx*vx+dx+vx)<pa) {
//                    for(k=0,os=other_size,od=other_des;k<other_cnt;k++,od++,os++) {
//                        if(!(od->x+*os<=tx || od->x>=tx+size) && !(od->y+*os<=ty || od->y>=ty+size)) break;
//                    }
//                    if(k>=other_cnt) { mx=tx; my=ty; return true; }
//                }
//            }
//            tx=mx-(i+1)+u; ty=my+(i+1);
//			if(IsAbsenceWall(nsh,size,tx,ty)) { 
//                float dx=GLOBAL_SCALE_MOVE*tx+GLOBAL_SCALE_MOVE*size/2-sx;
//                float dy=GLOBAL_SCALE_MOVE*ty+GLOBAL_SCALE_MOVE*size/2-sy;
//                float t=1.0f/sqrt(POW2(dx)+POW2(dy));
//                dx*=t; dy*=t;
//                if(fabs(dx*vx+dx*vx)<pa) {
//                    for(k=0,os=other_size,od=other_des;k<other_cnt;k++,od++,os++) {
//                        if(!(od->x+*os<=tx || od->x>=tx+size) && !(od->y+*os<=ty || od->y>=ty+size)) break;
//                    }
//                    if(k>=other_cnt) { mx=tx; my=ty; return true; }
//                }
//            }
//		}
//
//		for(u=0;u<(i*2+1);u++) {
//            tx=mx-(i+1); ty=my-i+u;
//			if(IsAbsenceWall(nsh,size,tx,ty)) { 
//                float dx=GLOBAL_SCALE_MOVE*tx+GLOBAL_SCALE_MOVE*size/2-sx;
//                float dy=GLOBAL_SCALE_MOVE*ty+GLOBAL_SCALE_MOVE*size/2-sy;
//                float t=1.0f/sqrt(POW2(dx)+POW2(dy));
//                dx*=t; dy*=t;
//                if(fabs(dx*vx+dx+vx)<pa) {
//                    for(k=0,os=other_size,od=other_des;k<other_cnt;k++,od++,os++) {
//                        if(!(od->x+*os<=tx || od->x>=tx+size) && !(od->y+*os<=ty || od->y>=ty+size)) break;
//                    }
//                    if(k>=other_cnt) { mx=tx; my=ty; return true; }
//                }
//            }
//            tx=mx+(i+1); ty=my-i+u;
//			if(IsAbsenceWall(nsh,size,tx,ty)) { 
//                float dx=GLOBAL_SCALE_MOVE*tx+GLOBAL_SCALE_MOVE*size/2-sx;
//                float dy=GLOBAL_SCALE_MOVE*ty+GLOBAL_SCALE_MOVE*size/2-sy;
//                float t=1.0f/sqrt(POW2(dx)+POW2(dy));
//                dx*=t; dy*=t;
//                if(fabs(dx*vx+dx+vx)<pa) {
//                    for(k=0,os=other_size,od=other_des;k<other_cnt;k++,od++,os++) {
//                        if(!(od->x+*os<=tx || od->x>=tx+size) && !(od->y+*os<=ty || od->y>=ty+size)) break;
//                    }
//                    if(k>=other_cnt) { mx=tx; my=ty; return true; }
//                }
//            }
//		}
//		i++;
//	}
//	return false;
//}

bool CMatrixMapLogic::PlaceFindNear(int nsh, int size, int &mx, int &my, CMatrixMapStatic *skip)
{
    int other_cnt = 0;
    int other_size[200];
    CPoint other_des[200];

    CMatrixMapStatic *obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->GetObjectType() == OBJECT_TYPE_ROBOT_AI && obj->AsRobot()->m_CurrentState != ROBOT_DIP && obj != skip)
        {
            CMatrixRobotAI *r = (CMatrixRobotAI*)obj;

            CPoint tp;
            if(r->GetReturnCoords(tp))
            {
                // ASSERT(other_cnt<200);
                if(other_cnt >= 200) return false;
                other_size[other_cnt] = 4;
                other_des[other_cnt] = tp;
                ++other_cnt;
            }
            if(r->GetMoveToCoords(tp))
            {
                // ASSERT(other_cnt<200);
                if(other_cnt >= 200) return false;
                other_size[other_cnt] = 4;
                other_des[other_cnt] = tp;
                ++other_cnt;
            }
            else
            {
                // ASSERT(other_cnt<200);
                if(other_cnt >= 200) return false;
                other_size[other_cnt] = 4;
                other_des[other_cnt] = CPoint(r->GetMapPosX(), r->GetMapPosY());
                ++other_cnt;
            }
        }
        else if(obj->GetObjectType() == OBJECT_TYPE_TURRET && obj->AsTurret()->m_CurrentState != TURRET_DIP && obj != skip)
        {
            // ASSERT(other_cnt<200);
            if(other_cnt >= 200) return false;

            other_size[other_cnt] = 4;
            other_des[other_cnt] = m_RoadNetwork.GetPlace(obj->AsTurret()->m_Place)->m_Pos;
            ++other_cnt;
        }
        obj = obj->GetNextLogic();
    }

    return PlaceFindNear(nsh, size, mx, my, other_cnt, other_size, other_des);
}

bool CMatrixMapLogic::PlaceFindNearReturn(int nsh, int size, int &mx, int &my, CMatrixRobotAI *robot)
{
    int other_cnt = 0;
    int other_size[200];
    CPoint other_des[200];

    for(int i = 0; i < robot->GetEnv()->m_BadCoordCnt; ++i)
    {
        // ASSERT(other_cnt<200);
        if(other_cnt >= 200) return false;
        other_size[other_cnt] = 4;
        other_des[other_cnt] = robot->GetEnv()->m_BadCoord[i];
        ++other_cnt;
    }

    CMatrixMapStatic *obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->GetObjectType() == OBJECT_TYPE_ROBOT_AI && obj->AsRobot()->m_CurrentState != ROBOT_DIP)
        {
            CMatrixRobotAI* r = (CMatrixRobotAI*)obj;

            // ASSERT(other_cnt < 200);
            if(other_cnt >= 200) return false;
            other_size[other_cnt] = 4;
            other_des[other_cnt].x = r->GetMapPosX();
            other_des[other_cnt].y = r->GetMapPosY();
            ++other_cnt;

            CPoint tp;
            if(r->GetReturnCoords(tp))
            {
                //ASSERT(other_cnt < 200);
                if(other_cnt >= 200) return false;
                other_size[other_cnt] = 4;
                other_des[other_cnt] = tp;
                ++other_cnt;
            }
            if(r->GetReturnCoords(tp))
            {
                //ASSERT(other_cnt < 200);
                if(other_cnt >= 200) return false;
                other_size[other_cnt] = 4;
                other_des[other_cnt] = tp;
                ++other_cnt;
            }
            if(r->GetMoveToCoords(tp))
            {
                //ASSERT(other_cnt < 200);
                if(other_cnt >= 200) return false;
                other_size[other_cnt] = 4;
                other_des[other_cnt] = tp;
                ++other_cnt;
            }
            else
            {
                //ASSERT(other_cnt < 200);
                if(other_cnt >= 200) return false;
                other_size[other_cnt] = 4;
                other_des[other_cnt] = CPoint(r->GetMapPosX(), r->GetMapPosY());
                ++other_cnt;
            }
        }
        else if(obj->GetObjectType() == OBJECT_TYPE_TURRET && obj->AsTurret()->m_CurrentState != TURRET_DIP)
        {
            //ASSERT(other_cnt < 200);
            if(other_cnt >= 200) return false;

            other_size[other_cnt] = 4;
            other_des[other_cnt] = m_RoadNetwork.GetPlace(obj->AsTurret()->m_Place)->m_Pos;
            ++other_cnt;
        }
        obj = obj->GetNextLogic();
    }

    return PlaceFindNear(nsh, size, mx, my, other_cnt, other_size, other_des);
}

//bool CMatrixMapLogic::PlaceFindNear(int nsh, int size, int &mx, int &my, const D3DXVECTOR2 &vdir, CMatrixMapStatic *skip)
//{
//    int other_cnt=0;
//    int other_size[50];
//    CPoint other_des[50];
//
//    CMatrixMapStatic * obj = CMatrixMapStatic::GetFirstLogic();
//    while(obj) {
//        if(obj->GetObjectType() == OBJECT_TYPE_ROBOT_AI && obj->AsRobot()->m_CurrentState != ROBOT_DIP && obj!=skip) {
//            CMatrixRobotAI * r=(CMatrixRobotAI*)obj;
//
//            CPoint tp;
//            if(r->GetReturnCoords(tp)) {
//                ASSERT(other_cnt<50);
//                other_size[other_cnt]=4;
//                other_des[other_cnt]=tp;
//                other_cnt++;
//            }
//            if(r->GetMoveToCoords(tp)) {
//                ASSERT(other_cnt<50);
//                other_size[other_cnt]=4;
//                other_des[other_cnt]=tp;
//                other_cnt++;
//            } else {
//                ASSERT(other_cnt<50);
//                other_size[other_cnt]=4;
//                other_des[other_cnt]=CPoint(r->m_MapX,r->m_MapY);
//                other_cnt++;
//            }
//        }
//        obj = obj->GetNextLogic();
//    }
//
//    return PlaceFindNear(nsh,size,mx,my,vdir,other_cnt,other_size,other_des);
//}

bool CMatrixMapLogic::PlaceIsEmpty(int nsh, int size, int mx, int my, CMatrixMapStatic *skip)
{
    DTRACE();

    CPoint tp;

    if(mx<0 || mx + size>m_SizeMove.x || my<0 || my + size>m_SizeMove.y) return false;

    ASSERT(size >= 1 && size <= 5);
    dword nsh_mask = (1 << nsh) << (6 * (size - 1));

    SMatrixMapMove *smm = MoveGet(mx, my);

    if ((smm->m_Stop & nsh_mask) != 0) return false;

    float kof = GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
    float cx = GLOBAL_SCALE_MOVE * mx + kof;
    float cy = GLOBAL_SCALE_MOVE * my + kof;

    CMatrixMapStatic *obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj != skip)
        {
            if(obj->GetObjectType() == OBJECT_TYPE_ROBOT_AI && obj->AsRobot()->m_CurrentState != ROBOT_DIP)
            {
                CMatrixRobotAI *r = (CMatrixRobotAI*)obj;

                if((POW2(cx - r->m_PosX) + POW2(cy - r->m_PosY)) < POW2(COLLIDE_BOT_R * 2)) return false;

                if(r->GetMoveToCoords(tp))
                {
                    if((POW2(cx - (GLOBAL_SCALE_MOVE * tp.x + kof)) + POW2(cy - (GLOBAL_SCALE_MOVE * tp.y + kof))) < POW2(COLLIDE_BOT_R * 2)) return false;
                }
                if (r->GetReturnCoords(tp))
                {
                    if((POW2(cx - (GLOBAL_SCALE_MOVE * tp.x + kof)) + POW2(cy - (GLOBAL_SCALE_MOVE * tp.y + kof))) < POW2(COLLIDE_BOT_R * 2)) return false;
                }
            }
        }
        obj = obj->GetNextLogic();
    }

    return true;
}

/*int CMatrixMapLogic::ZoneMoveFindNear(int nsh, int mx, int my, CPoint *path)
{
	int i;

	SMatrixMapUnit * mu=UnitGetTest(mx,my);
	if(!mu) ERROR_E;

	if(mu->m_Zone[nsh]>=0) return 0;

	int mind=1000000000;
	int minz=-1;
	for(i=0;i<m_ZoneCnt[nsh];i++) {
		int d=(mx-m_Zone[nsh][i].m_CenterX)*(mx-m_Zone[nsh][i].m_CenterX)+(my-m_Zone[nsh][i].m_CenterY)*(my-m_Zone[nsh][i].m_CenterY);
		if(d<mind) { mind=d; minz=i; }
	}

	if(minz<0) return 0;

	path[0].x=m_Zone[nsh][minz].m_CenterX;
	path[0].y=m_Zone[nsh][minz].m_CenterY;
	return 1;
}*/

/*int CMatrixMapLogic::ZoneMoveFind(int nsh,int size,int mx,int my,int zonesou1,int zonesou2,int zonesou3,int deszone,int dx,int dy,CPoint * path)
{
    DTRACE();

	int x,y,i,u;

	SMatrixMapMove * smm2,* smm=MoveGetTest(mx,my);
	if(!smm) ERROR_E;

	CRect re=m_RoadNetwork.m_Zone[zonesou1].m_Rect;
	UnionRect(&re,&re,&(m_RoadNetwork.m_Zone[zonesou2].m_Rect));
	if(zonesou3>=0)	UnionRect(&re,&re,&(m_RoadNetwork.m_Zone[zonesou3].m_Rect));
	UnionRect(&re,&re,&(m_RoadNetwork.m_Zone[deszone].m_Rect));
	re.left=max(re.left-size,0);
	re.top=max(re.top-size,0);
	re.right=min(re.right+size,m_SizeMove.x-size);
	re.bottom=min(re.bottom+size,m_SizeMove.y-size);

	smm=MoveGet(re.left,re.top);
	for(y=re.top;y<re.bottom;y++,smm+=m_SizeMove.x-(re.right-re.left)) {
		for(x=re.left;x<re.right;x++,smm++) {
			smm->m_Find=-1;
		}
	}

	if(!m_MPF) m_MPF=(CPoint *)HAlloc(sizeof(CPoint)*(m_SizeMove.x*2+m_SizeMove.y*2)*2,g_MatrixHeap);
	if(!m_MPF2) m_MPF2=(CPoint *)HAlloc(sizeof(CPoint)*(m_SizeMove.x*2+m_SizeMove.y*2)*2,g_MatrixHeap);
	m_MPFCnt=0; m_MPF2Cnt=0;

	MoveGetTest(mx,my)->m_Find=0;
	m_MPF[0].x=mx;
	m_MPF[0].y=my;
	m_MPFCnt++;

	bool fok=false;
	int level=1;

//HelperDestroyByGroup(dword(this));

	while(m_MPFCnt>0) {
		for(i=0;i<m_MPFCnt && !fok;i++) {
			for(u=0;u<4 && !fok;u++) {
				int nx=m_MPF[i].x+MatrixDir45[u].x;
				int ny=m_MPF[i].y+MatrixDir45[u].y;

//if(UnitGet(nx,ny)->m_Zone[nsh]==deszone) {
//ASSERT(1);
//}
//				if(nx<re.left || nx+size>re.right || ny<re.top || ny+size>=re.bottom) continue;
				if(nx<re.left || nx>=re.right || ny<re.top || ny>=re.bottom) continue;
				smm=MoveGet(nx,ny);
				if(smm->m_Find>=0) continue;

				smm2=smm;
				for(y=0;y<size;y++,smm2+=m_SizeMove.x-size) {
					for(x=0;x<size;x++,smm2++) {
						if(smm2->m_Stop & (1<<nsh)) break;
					}
					if(x<size) break;
				}
				if(y<size) continue;
				else if(nx==dx && ny==dy) {
					fok=true;
				} else if(smm->m_Zone==deszone && zonesou1!=deszone && zonesou2!=deszone && (zonesou3<0 || zonesou3!=deszone)) {
					fok=true;
				}
//				else if(smm->m_Zone!=zonesou1 && smm->m_Zone!=zonesou2 && (zonesou3<0 || smm->m_Zone!=zonesou3)) continue;

				smm->m_Find=level;

//CHelper::Create(0,dword(this))->Cone(D3DXVECTOR3(GLOBAL_SCALE*nx+5.0f,GLOBAL_SCALE*ny+5.0f,0),D3DXVECTOR3(10.0f*nx+5.0f,10.0f*ny+5.0f,10.0f+2.0f*level),0.5f,0.5f,0xffffffff,fok?0xff00ff00:0xffff0000,6);

				m_MPF2[m_MPF2Cnt].x=nx;
				m_MPF2[m_MPF2Cnt].y=ny;
				m_MPF2Cnt++;
			}
		}
		if(fok) break;

		level++;
		CPoint *tp=m_MPF; m_MPF=m_MPF2; m_MPF2=tp;
		m_MPFCnt=m_MPF2Cnt;
		m_MPF2Cnt=0;
	}

 	if(!fok) return 0;

	ASSERT(level+1<=MatrixPathMoveMax);

	x=m_MPF2[m_MPF2Cnt-1].x;
	y=m_MPF2[m_MPF2Cnt-1].y;

	path[level].x=x;
	path[level].y=y;

	int cnt=level+1;

	while(level>0) {
		for(u=0;u<8;u++) {
			int nx=x+MatrixDir45[u].x;
			int ny=y+MatrixDir45[u].y;

			if(nx<re.left || nx>=re.right || ny<re.top || ny>=re.bottom) continue;
			smm=MoveGet(nx,ny);
			if(smm->m_Find!=level-1) continue;
			x=nx; y=ny;
			level--;

			path[level].x=x;
			path[level].y=y;
			break;
		}
		ASSERT(u<8);
	}
	return cnt;
}*/

/*int CMatrixMapLogic::ZoneFindPath(int nsh,int zstart,int zend,int * path)
{
    DTRACE();

	int i,u;

	if(zstart==zend) return 0;

	for(i=0;i<m_RoadNetwork.m_ZoneCnt;i++) {
		m_RoadNetwork.m_Zone[i].m_FPLevel=-1;
		m_RoadNetwork.m_Zone[i].m_FPWt=1;
		m_RoadNetwork.m_Zone[i].m_FPWtp=0;
	}

	int * fp=(int *)HAlloc(m_RoadNetwork.m_ZoneCnt*sizeof(int *),g_MatrixHeap);
	int * fp2=(int *)HAlloc(m_RoadNetwork.m_ZoneCnt*sizeof(int *),g_MatrixHeap);
	int fpcnt=0;
	int fpcnt2=0;
	int level=0;

	fp[fpcnt]=zend;
	fpcnt++;
	m_RoadNetwork.m_Zone[zend].m_FPLevel=level;
	m_RoadNetwork.m_Zone[zend].m_FPWtp=0;
	level++;

	bool fok=false;

//HelperDestroyByGroup(2);

	while(fpcnt>0) {
		for(i=0;i<fpcnt;i++) {
			for(u=0;u<m_RoadNetwork.m_Zone[fp[i]].m_NearZoneCnt;u++) {
				int nz=m_RoadNetwork.m_Zone[fp[i]].m_NearZone[u];
				int wtp=m_RoadNetwork.m_Zone[fp[i]].m_FPWtp+m_RoadNetwork.m_Zone[nz].m_FPWt;

                if(m_RoadNetwork.m_Zone[nz].m_Move & (1<<nsh)) continue;

				if(m_RoadNetwork.m_Zone[nz].m_FPLevel<0 || wtp<m_RoadNetwork.m_Zone[nz].m_FPWtp) {
					m_RoadNetwork.m_Zone[nz].m_FPLevel=level;
					m_RoadNetwork.m_Zone[nz].m_FPWtp=wtp;
					fp2[fpcnt2]=nz;
					fpcnt2++;

//CHelper::Create(0,2)->Cone(D3DXVECTOR3(GLOBAL_SCALE_MOVE*m_RoadNetwork.m_Zone[nz].m_CenterX,GLOBAL_SCALE_MOVE*m_RoadNetwork.m_Zone[nz].m_CenterY,0),D3DXVECTOR3(GLOBAL_SCALE_MOVE*m_RoadNetwork.m_Zone[nz].m_CenterX,GLOBAL_SCALE*m_RoadNetwork.m_Zone[nz].m_CenterY,GLOBAL_SCALE_MOVE+wtp*2.0f),0.5f,0.5f,0xffffffff,0xffff0000,6);

					if(zstart==nz) fok=true;
				}
			}
		}

		level++;
		fpcnt=fpcnt2;
		fpcnt2=0;
		int * fpt=fp; fp=fp2; fp2=fpt;
	}

	HFree(fp,g_MatrixHeap);
	HFree(fp2,g_MatrixHeap);

	if(!fok) return 0;

	path[0]=zstart;
	int cnt=1;
	int curzone=zstart;
	int curwt=m_RoadNetwork.m_Zone[curzone].m_FPWtp;
	while(true) {
		int nz_=-1;
		int wtp_=curwt;
		for(int i=0;i<m_RoadNetwork.m_Zone[curzone].m_NearZoneCnt;i++) {
			int nz=m_RoadNetwork.m_Zone[curzone].m_NearZone[i];
			int wtp=m_RoadNetwork.m_Zone[nz].m_FPWtp;
            
            if(m_RoadNetwork.m_Zone[nz].m_Move & (1<<nsh)) continue;

			if(nz==zend) { nz_=nz; wtp_=wtp; break; }
			else if(wtp<=wtp_) { nz_=nz; wtp_=wtp; }
		}
		if(nz_<0) {
			ERROR_E;
		}

		curzone=nz_;
		curwt=wtp_;
//if(!(cnt+1<=m_RoadNetwork.m_ZoneCnt)) {
//    ASSERT(1);
//}
		ASSERT(cnt+1<=m_RoadNetwork.m_ZoneCnt);
		path[cnt]=curzone;
		cnt++;
		if(curzone==zend) break;
	}

	return cnt;
}*/

void CMatrixMapLogic::SetWeightFromTo(int size, int x1, int y1, int x2, int y2)
{
    ASSERT(size >= 1 && size <= 5);

    if(x1 - size - 1 < 0 || x1 + size + 1 >= m_SizeMove.x ||
        x2 - size - 1 < 0 || x2 + size + 1 >= m_SizeMove.x ||
        y1 - size - 1 < 0 || y1 + size + 1 >= m_SizeMove.y ||
        y2 - size - 1 < 0 || y2 + size + 1 >= m_SizeMove.y) return;

    int listsme[6 * 2 + 1];
    int listWeight[6 * 2 + 1];
    int listsmecnt = 0;

    float vx = float(x2 - x1);
    float vy = float(y2 - y1);
    float vd = 1 / (float)sqrt(vx * vx + vy * vy);
    vx *= vd; vy *= vd;
    float s = 1.0f;

    listsme[listsmecnt] = 0;
    listWeight[listsmecnt] = 10;
    ++listsmecnt;

    for(int i = 1; i < size + 1; ++i, s += 1.0f)
    {
        listsme[listsmecnt] = int(0.0f - vy * s) + int(0.0f + vx * s) * m_SizeMove.x;
        listWeight[listsmecnt] = 10 + (size + 1 - i) * 10;
        ++listsmecnt;

        listsme[listsmecnt] = int(0.0f + vy * s) + int(0.0f - vx * s) * m_SizeMove.x;
        listWeight[listsmecnt] = 10 + (size + 1 - i) * 10;
        ++listsmecnt;
    }

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x2 >= x1 ? 1 : -1;
    int sy = y2 >= y1 ? m_SizeMove.x : -m_SizeMove.x;

    SMatrixMapMove *smm = MoveGet(x1, y1);//(byte *)bufdes + x1 * 4 + y1 * ll;
    SMatrixMapMove *smmt;

    if(dy <= dx)
    {
        int d = (dy << 1) - dx;
        int d1 = dy << 1;
        int d2 = (dy - dx) << 1;
        //*(dword *)obuf = color;
        smm += sx;
        for(int i = 1; i <= dx; ++i, smm += sx)
        {
            if(d > 0)
            {
                d += d2;
                smm += sy;
            }
            else d += d1;

            for(int i = 0; i < listsmecnt; ++i)
            {
                smmt = smm + listsme[i];
                if(smmt->m_Weight < listWeight[i]) smmt->m_Weight = listWeight[i];
            }
        }
    }
    else
    {
        int d = (dx << 1) - dy;
        int d1 = dx << 1;
        int d2 = (dx - dy) << 1;
        //*(dword *)obuf = color;
        smm += sy;
        for(int i = 1; i <= dy; ++i, smm += sy)
        {
            if(d > 0)
            {
                d += d2;
                smm += sx;
            }
            else d += d1;

            for(int i = 0; i < listsmecnt; ++i)
            {
                smmt = smm + listsme[i];
                if(smmt->m_Weight < listWeight[i]) smmt->m_Weight = listWeight[i];
            }
        }
    }
}

int CMatrixMapLogic::FindLocalPath(
    int nsh, int size,
    int mx, int my, // Начальная точка
    int *zonepath, int zonepathcnt, // Список зон через которые нужной найти путь
    int dx, int dy, // Точка назначения
    CPoint *path, // Рассчитанный путь
    int other_cnt, // Кол-во путей от других роботов
    int *other_size,           // Список размеров в других путях
    CPoint **other_path_list, // Список указателей на другие пути
    int *other_path_cnt,       // Список кол-во элементов в других путях
    CPoint *other_des,          // Список конечных точек в других путях
    bool test)
{
    SMatrixMapMove *smm2, *smm;
    int i, u, x, y, sme, cnt, next, level, findok, findbest;
    CPoint tp, tpfind;

    ASSERT(zonepathcnt >= 1);

    if(!MoveGetTest(mx, my))
    {
        ERROR_S(L"CMatrixMapLogic::FindLocalPath Error 1!");
    }

    if(!m_MapPoint) m_MapPoint = (CPoint*)HAlloc(m_SizeMove.x * m_SizeMove.y * sizeof(CPoint), Base::g_MatrixHeap);

    int zoneskipcnt = min(zonepathcnt - 1, 6);

    CRect re = m_RoadNetwork.m_Zone[zonepath[zoneskipcnt]].m_Rect;
    for(i = 0; i < zoneskipcnt; ++i)
    {
        UnionRect(&re, &re, &(m_RoadNetwork.m_Zone[zonepath[i]].m_Rect));
    }
    re.left = min(mx, re.left);
    re.top = min(my, re.top);
    re.right = max(mx + 1, re.right);
    re.bottom = max(my + 1, re.bottom);

    re.left = max(re.left - size, 0);
    re.top = max(re.top - size, 0);
    re.right = min(re.right + size, m_SizeMove.x - size);
    re.bottom = min(re.bottom + size, m_SizeMove.y - size);

    ASSERT(!re.IsEmpty());

    smm = MoveGet(re.left, re.top);
    for(y = re.top; y < re.bottom; ++y, smm += m_SizeMove.x - (re.right - re.left))
    {
        for(x = re.left; x < re.right; ++x, smm++)
        {
            smm->m_Find = -1;
            smm->m_Weight = 5;
        }
    }

    for(i = 0; i < other_cnt; ++i)
    {
        //Робот идет по маршруту 10%-60%
        /*
        CPoint * pl = other_path_list[i];
        for(u = 1; u < other_path_cnt[i]; ++u, ++pl)
        {
            SetWeightFromTo(other_size[i], pl->x, pl->y, (pl + 1)->x, (pl + 1)->y);
        }
        */
        // Куда робот становится 200%
        int sx = max(0, other_des[i].x - (other_size[i] - 1));
        int sy = max(0, other_des[i].y - (other_size[i] - 1));
        int ex = min(m_SizeMove.x, other_des[i].x + other_size[i]);
        int ey = min(m_SizeMove.y, other_des[i].y + other_size[i]);
        smm = MoveGet(sx, sy);
        for(y = sy; y < ey; ++y, smm += m_SizeMove.x - (ey - sy))
        {
            for(x = sx; x < ex; ++x, ++smm)
            {
                if(smm->m_Weight < 200) smm->m_Weight = 200;
            }
        }
        // Где робот стоит 30%
        if(other_path_cnt[i] > 0)
        {
            int sx = max(0, other_path_list[i]->x - (other_size[i] - 1));
            int sy = max(0, other_path_list[i]->y - (other_size[i] - 1));
            int ex = min(m_SizeMove.x, other_path_list[i]->x + other_size[i]);
            int ey = min(m_SizeMove.y, other_path_list[i]->y + other_size[i]);
            smm = MoveGet(sx, sy);
            for(y = sy; y < ey; ++y, smm += m_SizeMove.x - (ey - sy))
            {
                for(x = sx; x < ex; ++x, ++smm)
                {
                    if(smm->m_Weight < 30) smm->m_Weight = 30;
                }
            }
        }
    }

#if (defined _DEBUG) &&  !(defined _RELDEBUG) &&  !(defined _DISABLE_AI_HELPERS)
    if(test && g_TestLocal)
    {
        CHelper::DestroyByGroup(dword(this) + 4);
        smm = MoveGet(re.left, re.top);
        for(y = re.top; y < re.bottom; ++y, smm += m_SizeMove.x - (re.right - re.left))
        {
            for(x = re.left;x < re.right; ++x, ++smm)
            {
                if(smm->m_Weight >= 10) CHelper::Create(100, dword(this) + 4)->Cone(D3DXVECTOR3(GLOBAL_SCALE_MOVE * x + GLOBAL_SCALE_MOVE / 2, GLOBAL_SCALE_MOVE * y + GLOBAL_SCALE_MOVE / 2, 0), D3DXVECTOR3(GLOBAL_SCALE_MOVE * x + GLOBAL_SCALE_MOVE / 2, GLOBAL_SCALE_MOVE * y + GLOBAL_SCALE_MOVE / 2, 0.5f), 0.5f, (GLOBAL_SCALE_MOVE / 2.0f) * ((float)smm->m_Weight) / 200.0f, 0x800000ff, 0x800000ff, 8);
            }
    }
}
#endif


    sme = 0;
    cnt = 1;
    level = 1;
    m_MapPoint[0].x = mx;
    m_MapPoint[0].y = my;
    next = cnt;
    smm = MoveGet(mx, my);
    smm->m_Find = 0;
    findok = 0;
    findbest = -1;

    ASSERT(size >= 1 && size <= 5);
    dword nsh_mask = (1 << nsh) << (6 * (size - 1));

    //CHelper::DestroyByGroup(dword(this) + 3);

        //zakker stuff. anticrash
    int maxmax = m_SizeMove.x * m_SizeMove.y;

    while(sme < cnt)
    {

        //if(sme >= maxmax || cnt >=maxmax) break;

        smm2 = MoveGet(m_MapPoint[sme].x, m_MapPoint[sme].y);
        for(u = 0; u < 4; ++u)
        {
            tp = m_MapPoint[sme];
            switch(u)
            {
            case 0:
                --tp.x;
                if(tp.x < re.left) continue;
                break;
            case 1:
                --tp.y;
                if(tp.y < re.top) continue;
                break;
            case 2:
                ++tp.x;
                if(tp.x >= re.right) continue;
                break;
            case 3:
                ++tp.y;
                if(tp.y >= re.bottom) continue;
                break;
            }
            smm = MoveGet(tp.x, tp.y);
            if(sme != 0 && smm->m_Stop & nsh_mask) continue;
            if(smm->m_Find >= 0 && (smm2->m_Find + smm->m_Weight) >= smm->m_Find) continue;

            if(!findok && tp.x == dx && tp.y == dy)
            {
                tpfind.x = dx;
                tpfind.y = dy;
                findok = 1;
            }
            else if((!findok || findbest >= 0) && (zoneskipcnt + 1) < zonepathcnt && smm->m_Zone == zonepath[zoneskipcnt])
            {
                if(findbest >= 0 && smm2->m_Find + smm->m_Weight >= findbest) continue;
                findbest = smm2->m_Find + smm->m_Weight;
                tpfind = tp;
                findok = 1;
            }

#if (defined _DEBUG) &&  !(defined _RELDEBUG) && !(defined _DISABLE_AI_HELPERS)
            if(test && g_TestLocal)
            {
                CHelper::Create(100, dword(this) + 3)->Cone(D3DXVECTOR3(GLOBAL_SCALE_MOVE * tp.x + GLOBAL_SCALE_MOVE / 2, GLOBAL_SCALE_MOVE * tp.y + GLOBAL_SCALE_MOVE / 2, 0), D3DXVECTOR3(GLOBAL_SCALE_MOVE * tp.x + GLOBAL_SCALE_MOVE / 2, GLOBAL_SCALE_MOVE * tp.y + GLOBAL_SCALE_MOVE / 2, 10.0f + 2.0f * level), 0.5f, 0.5f, 0xffffffff, (tpfind.x == tp.x && tpfind.y == tp.y) ? 0xff00ff00 : 0xffff0000, 6);
            }
#endif

            //#if (defined _DEBUG) && !(defined _RELDEBUG)
                        //            if(cnt >= maxmax) __asm int 3;
            //#endif
            if(cnt >= maxmax) break;

            smm->m_Find = smm2->m_Find + smm->m_Weight;
            m_MapPoint[cnt] = tp;
            ++cnt;
        }
        if(cnt >= maxmax) break;
        ++sme;
        if(sme >= next)
        {
            next = cnt;
            ++level;
            if(findok)
            {
                ++findok;
                if(findok > 16) break;
            }
        }
    }

    if(!findok) return 0;

    cnt = 0;
    path[MatrixPathMoveMax - 1 - cnt] = tpfind;
    ++cnt;
    smm = MoveGet(tpfind.x, tpfind.y);
    int lastangle = 0;

    while(smm->m_Find)
    {
        CPoint tpbest;
        int anglebest;
        SMatrixMapMove* smmbest = nullptr;

        for(u = 0; u < 4; ++u)
        {
            tp = tpfind;
            switch((u + lastangle) & 3)
            {
            case 0: tp.x--; if (tp.x < re.left) continue; smm2 = smm - 1; break;
            case 1: tp.y--; if (tp.y < re.top) continue; smm2 = smm - m_SizeMove.x; break;
            case 2: tp.x++; if (tp.x >= re.right) continue; smm2 = smm + 1; break;
            case 3: tp.y++; if (tp.y >= re.bottom) continue; smm2 = smm + m_SizeMove.x; break;
            }
            if(smm2->m_Find < 0) continue;
            if(smmbest)
            {
                if(smmbest->m_Find <= smm2->m_Find) continue;
            }
            anglebest = (u + lastangle) & 3;
            tpbest = tp;
            smmbest = smm2;
        }

        if(!smmbest) ERROR_S(L"CMatrixMapLogic::FindLocalPath Error 2!");

        smm = smmbest;
        lastangle = anglebest;
        tpfind = tpbest;

        if(MatrixPathMoveMax - 1 - cnt <= 0) ERROR_S(L"CMatrixMapLogic::FindLocalPath Error 3!");
        path[MatrixPathMoveMax - 1 - cnt] = tpfind;
        ++cnt;
    }

    u = MatrixPathMoveMax - 1 - cnt + 1;
    if(u > 0) MoveMemory(path, path + u, cnt * sizeof(CPoint));

    //cnt = g_MatrixMap->OptimizeMovePath(nsh, size, cnt, path);

    return cnt;
}

void CMatrixMapLogic::SetZoneAccess(int *list, int cnt, bool value)
{
    while(cnt)
    {
        m_RoadNetwork.m_Zone[*list].m_Access = value;
        ++list;
        --cnt;
    }
}

int CMatrixMapLogic::FindPathInZone(int nsh, int zstart, int zend, const CMatrixRoadRoute *route, int routeno, int *path, bool test)
{
#if (defined _DEBUG) &&  !(defined _RELDEBUG)
    if(test && !g_TestLocal) CHelper::DestroyByGroup(100);
#endif

    if(zstart == zend) return 0;

    PrepareBuf();

    for(int i = 0; i < m_RoadNetwork.m_Zone[zstart].m_NearZoneCnt; ++i) //Если конец в соседней зоне
    {
        if(m_RoadNetwork.m_Zone[zstart].m_NearZone[i] == zend)
        {
            path[0] = zstart;
            path[1] = zend;
            return 2;
        }
    }

    int accesscnt = 0;

    if(route) //Устанавливаем доступ
    {
        for(int i = 1; i < route->m_Header[routeno].m_Cnt; ++i)
        {
            CMatrixRoad *road = route->m_Modules[routeno * m_RoadNetwork.m_CrotchCnt + i].m_Road;

            for(int u = 0; u < road->m_ZoneCnt; ++u)
            {
                SMatrixMapZone &z1 = m_RoadNetwork.m_Zone[road->m_Zone[u]];
                if(z1.m_Move & (1 << nsh)) continue;
                if(!z1.m_Access)
                {
                    z1.m_Access = true;
                    m_ZoneIndexAccess[accesscnt] = road->m_Zone[u];
                    ++accesscnt;
                }

                for(int t = 0; t < z1.m_NearZoneCnt; ++t)
                {
                    if(z1.m_NearZoneMove[t] & (1 << nsh)) continue;
                    SMatrixMapZone &z2 = m_RoadNetwork.m_Zone[z1.m_NearZone[t]];
                    if(z2.m_Move & (1 << nsh)) continue;
                    if(!z2.m_Access)
                    {
                        z2.m_Access = true;
                        m_ZoneIndexAccess[accesscnt] = z1.m_NearZone[t];
                        ++accesscnt;
                    }

                    for(int k = 0; k < z2.m_NearZoneCnt; ++k)
                    {
                        if(z2.m_NearZoneMove[k] & (1 << nsh)) continue;
                        SMatrixMapZone &z3 = m_RoadNetwork.m_Zone[z2.m_NearZone[k]];
                        if(z3.m_Move & (1 << nsh)) continue;
                        if(!z3.m_Access)
                        {
                            z3.m_Access = true;
                            m_ZoneIndexAccess[accesscnt] = z2.m_NearZone[k];
                            ++accesscnt;
                        }
                    }
                }
            }
        }
    }

    int curzone, newzone, cnt, u, sme, level, next, zonefindok;
    SMatrixMapZone *zone2;

    cnt = m_RoadNetwork.m_ZoneCnt;
    SMatrixMapZone *zone = m_RoadNetwork.m_Zone;
    while(cnt)
    {
        zone->m_FPLevel = 0;
        ++zone;
        --cnt;
    }

    // Ищем путь от конца до дороги или до начально зоны
    sme = 0;
    level = 1;
    cnt = 1;
    m_ZoneIndex[0] = zend;
    m_RoadNetwork.m_Zone[zend].m_FPLevel = level;
    ++level;
    next = cnt;

    zonefindok = -1;
    while(sme < cnt)
    {
        zone = m_RoadNetwork.m_Zone + m_ZoneIndex[sme];
        for(int i = 0; i < zone->m_NearZoneCnt; ++i)
        {
            if(zone->m_NearZoneMove[i] & (1 << nsh)) continue;
            newzone = zone->m_NearZone[i];
            zone2 = m_RoadNetwork.m_Zone + newzone;
            if(newzone == zstart || zone2->m_Access) //Нашли
            {
                zonefindok = newzone;
                sme = cnt;
                break;
            }
            if(zone2->m_FPLevel) continue;
            if(zone2->m_Move & (1 << nsh)) continue;

            m_ZoneIndex[cnt] = newzone;
            ++cnt;
            zone2->m_FPLevel = level;
        }

        ++sme;
        if(sme >= next)
        {
            next = cnt;
            ++level;
        }
    }

    if(zonefindok < 0) //Не нашли ни какого пути
    {
        SetZoneAccess(m_ZoneIndexAccess, accesscnt, false);
        return 0;

    }
    else if(zonefindok == zstart) //Нашли путь до начальной зоны
    {
        path[0] = zstart;
        cnt = 1;
        curzone = zstart;

        while(curzone != zend)
        {
            zone = m_RoadNetwork.m_Zone + curzone;
            int i;
            for(i = 0; i < zone->m_NearZoneCnt; ++i)
            {
                if(zone->m_NearZoneMove[i] & (1 << nsh)) continue;
                newzone = zone->m_NearZone[i];
                zone2 = m_RoadNetwork.m_Zone + newzone;
                if(zone2->m_FPLevel <= 0) continue;
                if(zone2->m_FPLevel < level)
                {
                    curzone = newzone;
                    path[cnt] = newzone;
                    ++cnt;
                    level = zone2->m_FPLevel;
                    break;
                }
            }

            if(i >= zone->m_NearZoneCnt) ERROR_S(L"CMatrixMapLogic::FindPathInZone Error 1!");
        }

        SetZoneAccess(m_ZoneIndexAccess, accesscnt, false);
        return cnt;
    }
    else //Нашли путь до дороги
    {
        curzone = zonefindok;
        zone = m_RoadNetwork.m_Zone + curzone;

        while(curzone != zend)
        {
            bool ok = false;
            for(int i = 0; i < zone->m_NearZoneCnt; ++i)
            {
                if(zone->m_NearZoneMove[i] & (1 << nsh)) continue;
                newzone = zone->m_NearZone[i];
                zone2 = m_RoadNetwork.m_Zone + newzone;
                if(zone2->m_FPLevel <= 0) continue;
                if(zone2->m_FPLevel < level)
                {
                    curzone = newzone;
                    level = zone2->m_FPLevel;

                    if(!zone2->m_Access)
                    {
                        zone2->m_Access = true;
                        m_ZoneIndexAccess[accesscnt] = newzone;
                        ++accesscnt;
                    }
                    zone = m_RoadNetwork.m_Zone + curzone;
                    for(u = 0; u < zone->m_NearZoneCnt; ++u)
                    {
                        if(zone->m_NearZoneMove[u] & (1 << nsh)) continue;
                        zone2 = m_RoadNetwork.m_Zone + zone->m_NearZone[u];
                        if(zone2->m_Move & (1 << nsh)) continue;
                        if(!zone2->m_Access)
                        {
                            zone2->m_Access = true;
                            m_ZoneIndexAccess[accesscnt] = zone->m_NearZone[u];
                            ++accesscnt;
                        }
                    }
                    ok = true;
                    break;
                }
            }

            if(!ok/*i >= zone->m_NearZoneCnt*/) ERROR_S(L"CMatrixMapLogic::FindPathInZone Error 2!");
        }
    }

    //Ищем путь от начала до дороги
    for(int i = 0; i < cnt; ++i)
    {
        m_RoadNetwork.m_Zone[m_ZoneIndex[i]].m_FPLevel = 0;
    }

    sme = 0;
    level = 1;
    cnt = 1;
    m_ZoneIndex[0] = zstart;
    m_RoadNetwork.m_Zone[zstart].m_FPLevel = level;
    ++level;
    next = cnt;

    zonefindok = -1;
    while(sme < cnt)
    {
        zone = m_RoadNetwork.m_Zone + m_ZoneIndex[sme];
        for(int i = 0; i < zone->m_NearZoneCnt; ++i)
        {
            if(zone->m_NearZoneMove[i] & (1 << nsh)) continue;
            newzone = zone->m_NearZone[i];
            zone2 = m_RoadNetwork.m_Zone + newzone;
            if(zone2->m_Access) //Нашли
            {
                zonefindok = newzone;
                sme = cnt;
                break;
            }
            if(zone2->m_FPLevel) continue;
            if(zone2->m_Move & (1 << nsh)) continue;

            m_ZoneIndex[cnt] = newzone;
            ++cnt;
            zone2->m_FPLevel = level;
        }

        ++sme;
        if(sme >= next)
        {
            next = cnt;
            ++level;
        }
    }

    if(zonefindok < 0) //Не нашли никакого пути
    {
        SetZoneAccess(m_ZoneIndexAccess, accesscnt, false);
        return 0;
    }
    else //Нашли путь до дороги
    {
        curzone = zonefindok;
        zone = m_RoadNetwork.m_Zone + curzone;

        while(curzone != zstart)
        {
            bool ok = false;
            for(int i = 0; i < zone->m_NearZoneCnt; ++i)
            {
                if(zone->m_NearZoneMove[i] & (1 << nsh)) continue;
                newzone = zone->m_NearZone[i];
                zone2 = m_RoadNetwork.m_Zone + newzone;
                if(zone2->m_FPLevel <= 0) continue;
                if(zone2->m_FPLevel < level)
                {
                    curzone = newzone;
                    level = zone2->m_FPLevel;

                    if(!zone2->m_Access)
                    {
                        zone2->m_Access = true;
                        m_ZoneIndexAccess[accesscnt] = newzone;
                        ++accesscnt;
                    }
                    zone = m_RoadNetwork.m_Zone + curzone;
                    for(u = 0; u < zone->m_NearZoneCnt; ++u)
                    {
                        if(zone->m_NearZoneMove[u] & (1 << nsh)) continue;
                        zone2 = m_RoadNetwork.m_Zone + zone->m_NearZone[u];
                        if(zone2->m_Move & (1 << nsh)) continue;
                        if(!zone2->m_Access)
                        {
                            zone2->m_Access = true;
                            m_ZoneIndexAccess[accesscnt] = zone->m_NearZone[u];
                            ++accesscnt;
                        }
                    }
                    ok = true;
                    break;
                }
            }

            if(!ok/*i >= zone->m_NearZoneCnt*/) ERROR_S(L"CMatrixMapLogic::FindPathInZone Error 3!");
        }
    }

    //Ищем путь от конца до начала
    for(int i = 0; i < cnt; ++i) m_RoadNetwork.m_Zone[m_ZoneIndex[i]].m_FPLevel = 0;

#if (defined _DEBUG) && !(defined _RELDEBUG)
    if(test && !g_TestLocal)
    {
        for(int i = 0; i < m_RoadNetwork.m_ZoneCnt; ++i)
        {
            zone = m_RoadNetwork.m_Zone + i;
            D3DXVECTOR3 v;
            v.x = zone->m_Center.x * GLOBAL_SCALE_MOVE;
            v.y = zone->m_Center.y * GLOBAL_SCALE_MOVE;
            v.z = GetZ(v.x, v.y);
            if(zone->m_Access) CHelper::Create(0, 100)->Line(v, D3DXVECTOR3(v.x, v.y, v.z + 20.0f), 0xffffffff, 0xffffffff);
            else CHelper::Create(0, 100)->Line(v, D3DXVECTOR3(v.x, v.y, v.z + 20.0f), 0xffff0000, 0xffff0000);
        }
        for(int i = 0; i < route->m_Header[routeno].m_Cnt; ++i)
        {
            D3DXVECTOR3 v;
            v.x = route->m_Modules[routeno * m_RoadNetwork.m_CrotchCnt + i].m_Crotch->m_Center.x * GLOBAL_SCALE_MOVE;
            v.y = route->m_Modules[routeno * m_RoadNetwork.m_CrotchCnt + i].m_Crotch->m_Center.y * GLOBAL_SCALE_MOVE;
            v.z = GetZ(v.x, v.y);
            CHelper::Create(0, 100)->Line(D3DXVECTOR3(v.x, v.y, v.z + 50.0f), D3DXVECTOR3(v.x, v.y, v.z + 120.0f), 0xff00ff00, 0xff00ff00);

            CMatrixRoad* road = route->m_Modules[routeno * m_RoadNetwork.m_CrotchCnt + i].m_Road;

            if(road)
            {
                for(int u = 1; u < road->m_ZoneCnt; ++u)
                {
                    SMatrixMapZone& z1 = m_RoadNetwork.m_Zone[road->m_Zone[u - 1]];
                    SMatrixMapZone& z2 = m_RoadNetwork.m_Zone[road->m_Zone[u]];
                    D3DXVECTOR3 v1, v2;
                    v1.x = z1.m_Center.x * GLOBAL_SCALE_MOVE;
                    v1.y = z1.m_Center.y * GLOBAL_SCALE_MOVE;
                    v1.z = GetZ(v1.x, v1.y) + 50.0f;
                    v2.x = z2.m_Center.x * GLOBAL_SCALE_MOVE;
                    v2.y = z2.m_Center.y * GLOBAL_SCALE_MOVE;
                    v2.z = GetZ(v2.x, v2.y) + 50.0f;
                    CHelper::Create(0, 100)->Line(v1, v2, 0xff00ff00, 0xff00ff00);
                }
            }
        }
    }
#endif

    sme = 0;
    level = 1;
    cnt = 1;
    m_ZoneIndex[0] = zend;
    m_RoadNetwork.m_Zone[zend].m_FPLevel = level;
    ++level;
    next = cnt;

    zonefindok = -1;
    while(sme < cnt)
    {
        zone = m_RoadNetwork.m_Zone + m_ZoneIndex[sme];
        for(int i = 0; i < zone->m_NearZoneCnt; ++i)
        {
            if(zone->m_NearZoneMove[i] & (1 << nsh)) continue;
            newzone = zone->m_NearZone[i];
            zone2 = m_RoadNetwork.m_Zone + newzone;
            if(newzone == zstart) //Нашли
            {
                zonefindok = newzone;
                sme = cnt;
                break;
            }
            if(zone2->m_FPLevel) continue;
            if(!zone2->m_Access) continue;
            if(zone2->m_Move & (1 << nsh)) continue;

            m_ZoneIndex[cnt] = newzone;
            ++cnt;
            zone2->m_FPLevel = level;
        }

        ++sme;
        if(sme >= next)
        {
            next = cnt;
            ++level;
        }
    }

    if(zonefindok < 0) //Не нашли никакого пути
    {
        SetZoneAccess(m_ZoneIndexAccess, accesscnt, false);
        return 0;

    }
    else //Нашли полный путь
    {
        path[0] = zstart;
        cnt = 1;
        curzone = zstart;

        while(curzone != zend)
        {
            zone = m_RoadNetwork.m_Zone + curzone;
            int i;
            for(i = 0; i < zone->m_NearZoneCnt; ++i)
            {
                if(zone->m_NearZoneMove[i] & (1 << nsh)) continue;
                newzone = zone->m_NearZone[i];
                zone2 = m_RoadNetwork.m_Zone + newzone;
                if(zone2->m_FPLevel <= 0) continue;
                if(zone2->m_FPLevel < level)
                {
                    curzone = newzone;
                    path[cnt] = newzone;
                    ++cnt;
                    level = zone2->m_FPLevel;
                    break;
                }
            }

            if(i >= zone->m_NearZoneCnt) ERROR_S(L"CMatrixMapLogic::FindPathInZone Error 4!");
        }
    }

    SetZoneAccess(m_ZoneIndexAccess, accesscnt, false);

    return cnt;
}

bool CMatrixMapLogic::CanMoveFromTo(int nsh, int size, int x1, int y1, int x2, int y2, CPoint*)
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x2 >= x1 ? 1 : -1;
    int sy = y2 >= y1 ? m_SizeMove.x : -m_SizeMove.x;

    SMatrixMapMove* smm = MoveGet(x1, y1);//(byte *)bufdes + x1 * 4 + y1 * ll;
    //SMatrixMapMove * smmt;

    ASSERT(size >= 1 && size <= 5);
    dword nsh_mask = (1 << nsh) << (6 * (size - 1));

    if(dy <= dx)
    {
        int d = (dy << 1) - dx;
        int d1 = dy << 1;
        int d2 = (dy - dx) << 1;
        //*(dword *)obuf = color;
        smm += sx;
        for(int i = 1; i <= dx; ++i, smm += sx)
        {
            if(d > 0)
            {
                d += d2;
                smm += sy;
            }
            else d += d1;

            if(smm->m_Stop & nsh_mask) return false;
            /*
            if(size == 1)
            {
				if(smm->m_Stop & (1<<nsh)) return false;
			}
            else
            {
				smmt = smm;
				for(int y = 0; y < size; ++y, smmt += m_SizeMove.x - size)
                {
                    for(int x = 0; x < size; ++x, ++smmt)
                    {
                        if(smmt->m_Stop & (1 << nsh)) return false;
                    }
                }
			}
            */
		}
	}
    else
    {
        int d = (dx << 1) - dy;
        int d1 = dx << 1;
        int d2 = (dx - dy) << 1;
        //*(dword *)obuf = color;
        smm += sy;
        for(int i = 1; i <= dy; ++i, smm += sy)
        {
            if(d > 0)
            {
                d += d2;
                smm += sx;
            }
            else d += d1;

            if(smm->m_Stop & nsh_mask) return false;
            /*
            if(size == 1)
            {
                if(smm->m_Stop & (1 << nsh)) return false;
            }
            else
            {
                smmt = smm;
                for(int y = 0; y < size; ++y,smmt += m_SizeMove.x - size)
                {
                    for(int x = 0; x < size; ++x, ++smmt)
                    {
                        if(smmt->m_Stop & (1 << nsh)) return false;
                    }
                }
            }
            */
        }
    }
    return true;
}

int OptimizeMovePath_Delete(int cnt, CPoint *path, int from, int to)
{
    if(from >= to) return cnt;
    if(from < 0 || to >= cnt) return cnt;

    MoveMemory(path + from + 1, path + to, (cnt - to) * sizeof(CPoint));
    return cnt - ((to - from) - 1);
}

bool CMatrixMapLogic::CanOptimize(int nsh, int size, int x1, int y1, int x2, int y2)
{
    DTRACE();

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x2 >= x1 ? 1 : -1;
    int sy = y2 >= y1 ? m_SizeMove.x : -m_SizeMove.x;

    SMatrixMapMove *smm = MoveGet(x1, y1);//(byte *)bufdes + x1 * 4 + y1 * ll;
    SMatrixMapMove *smmt;

    ASSERT(size >= 1 && size <= 5);
    dword nsh_mask = (1 << nsh) << (6 * (size - 1));

    if(dy <= dx)
    {
        int d = (dy << 1) - dx;
        int d1 = dy << 1;
        int d2 = (dy - dx) << 1;
        //*(dword *)obuf = color;
        smm += sx;
        for(int i = 1; i <= dx; ++i, smm += sx)
        {
            if(d > 0)
            {
                d += d2;
                smm += sy;
            }
            else d += d1;

            if(smm->m_Stop & nsh_mask) return false;

            smmt = smm;
            for(int y = 0; y < size; ++y, smmt += m_SizeMove.x - size)
            {
                for(int x = 0; x < size; ++x, ++smmt)
                {
                    if(smmt->m_Weight >= 40) return false;
                }
            }
        }
    }
    else
    {
        int d = (dx << 1) - dy;
        int d1 = dx << 1;
        int d2 = (dx - dy) << 1;
        //*(dword *)obuf = color;
        smm += sy;
        for(int i = 1; i <= dy; ++i, smm += sy)
        {
            if(d > 0)
            {
                d += d2;
                smm += sx;
            }
            else d += d1;

            if(smm->m_Stop & nsh_mask) return false;

            smmt = smm;
            for(int y = 0; y < size; ++y, smmt += m_SizeMove.x - size)
            {
                for(int x = 0; x < size; ++x, ++smmt)
                {
                    if(smmt->m_Weight >= 40) return false;
                }
            }
        }
    }
	return true;
}

int CMatrixMapLogic::OptimizeMovePath(int nsh, int size, int cnt, CPoint *path)
{
    DTRACE();

    if(cnt <= 2) return cnt;

    struct {
        int ibegin, iend;
        bool loopagain;
    } os[MatrixPathMoveMax];
    int oscnt = 0;

    // Находим все углы (путь меняет направление)
    CPoint prevangle, curangle;
    prevangle.x = path[1].x - path[0].x;
    prevangle.y = path[1].y - path[0].y;

    os[oscnt].ibegin = 0;
    os[oscnt].iend = 0;
    os[oscnt].loopagain = true;
    ++oscnt;

    for(int i = 2; i < cnt; ++i)
    {
        curangle.x = path[i].x - path[i - 1].x;
        curangle.y = path[i].y - path[i - 1].y;

        if((curangle.x != prevangle.x) || (curangle.y != prevangle.y))
        {
            prevangle = curangle;
            os[oscnt].ibegin = i - 1;
            os[oscnt].iend = i - 1;
            os[oscnt].loopagain = true;
            ++oscnt;
        }
	}

    os[oscnt].ibegin = cnt - 1;
    os[oscnt].iend = cnt - 1;
    os[oscnt].loopagain = true;
    ++oscnt;

    /*
    if(oscnt <= 0)
    {
		path[1] = path[cnt - 1];
		return 2;
	}
    */

    //Выращиваем углы во все сторонный
    bool loopagain = true;
    while(loopagain)
    {
        loopagain = false;

        for(int i = 0; i < oscnt; ++i)
        {
            if(!os[i].loopagain) continue;
            os[i].loopagain = false;

            if((i == 0 && os[i].ibegin > 0) || (i != 0 && os[i].ibegin > os[i - 1].iend))
            {
                if(CanOptimize(nsh, size, path[os[i].ibegin - 1].x, path[os[i].ibegin - 1].y, path[os[i].iend].x, path[os[i].iend].y))
                {
                    loopagain = true;
                    os[i].loopagain = true;
                    --os[i].ibegin;
                }
            }
            if((i == (oscnt - 1) && os[i].iend < (cnt - 1)) || (i != (oscnt - 1) && os[i].iend < os[i + 1].ibegin))
            {
                if(CanOptimize(nsh, size, path[os[i].ibegin].x, path[os[i].ibegin].y, path[os[i].iend + 1].x, path[os[i].iend + 1].y))
                {
                    loopagain = true;
                    os[i].loopagain = true;
                    ++os[i].iend;
                }
            }
        }
    }

    //Удаляем повторяющиеся направления
    /*
    cnt = OptimizeMovePath_Delete(cnt, path, os[oscnt-1].iend, cnt - 1);
	if(oscnt == 1)
    {
		cnt = OptimizeMovePath_Delete(cnt, path, os[0].ibegin, os[0].iend);
	}
    else
    {*/
        for(int i = oscnt - 1; i >= 1; --i)
        {
            cnt = OptimizeMovePath_Delete(cnt, path, os[i].ibegin, os[i].iend);
            cnt = OptimizeMovePath_Delete(cnt, path, os[i - 1].iend, os[i].ibegin);
        }
    //}

    cnt = OptimizeMovePath_Delete(cnt, path, os[0].ibegin, os[0].iend);
    //cnt = OptimizeMovePath_Delete(cnt, path, 0, os[0].ibegin);

    cnt = OptimizeMovePathSimple(nsh, size, cnt, path);
    //cnt = RandomizeMovePath(nsh, size, cnt, path);

    return cnt;
}

int CMatrixMapLogic::OptimizeMovePathSimple(int nsh, int size, int cnt, CPoint *path)
{
    int to;
    int from = 0;
    while(from <= (cnt - 2))
    {
        for(to = from + 2; to < cnt; ++to)
        {
            if(!CanOptimize(nsh, size, path[from].x, path[from].y, path[to].x, path[to].y))
            {
                if((POW2(path[to - 1].x - path[to].x) + POW2(path[to - 1].y - path[to].y)) >= POW2(4))
                {
                    break;
                }
                else if((POW2(path[from].x - path[to].x) + POW2(path[from].y - path[to].y)) >= POW2(8))
                {
                    break;
                }
            }
        }
        --to;
        if((to - from) > 1)
        {
            MoveMemory(path + from + 1, path + to, (cnt - to) * sizeof(CPoint));
            cnt -= (to - from) - 1;
            ++from;
        }
        else from = to;
    }
    return cnt;
}

int CMatrixMapLogic::RandomizeMovePath(int nsh, int size, int cnt, CPoint *path)
{
    int zonelast = -1;

    for(int i = 1; i < (cnt - 1); ++i)
    {
        int zonecur = MoveGet(path[i].x, path[i].y)->m_Zone;
        if(zonelast == zonecur) continue;
        zonelast = zonecur;

        CRect re = m_RoadNetwork.m_Zone[zonecur].m_Rect;
        re.right -= size;
        re.bottom -= size;
        if(re.IsEmpty()) continue;

        int dist2 = (path[i].x - path[i - 1].x) * (path[i].x - path[i - 1].x) + (path[i].y - path[i - 1].y) * (path[i].y - path[i - 1].y);
        dist2 = min(dist2, (path[i].x - path[i + 1].x) * (path[i].x - path[i + 1].x) + (path[i].y - path[i + 1].y) * (path[i].y - path[i + 1].y));
        dist2 = min(dist2, 7 * 7);

        for(int u = 0; u < 5; ++u)
        {
            int newpx = Rnd(re.left, re.right);
            int newpy = Rnd(re.top, re.bottom);

            if(!PlaceFindNear(nsh, size, newpx, newpy, 0, nullptr, nullptr)) continue;
            if(newpx == path[i].x && newpy == path[i].y) break;
            if(zonecur != MoveGet(newpx, newpy)->m_Zone) continue;

            if(((newpx - path[i].x) * (newpx - path[i].x) + (newpy - path[i].y) * (newpy - path[i].y)) > dist2) continue;

            if(!CanMoveFromTo(nsh, size, path[i - 1].x, path[i - 1].y, newpx, newpy, path)) continue;
            if(!CanMoveFromTo(nsh, size, path[i + 1].x, path[i + 1].y, newpx, newpy, path)) continue;

            path[i].x = newpx;
            path[i].y = newpy;
            break;
        }
    }

    return cnt;
}

int CMatrixMapLogic::FindNearPlace(byte mm, const CPoint& mappos)
{
    int sme = 0;
    int cnt = 0;

    SMatrixMapMove* smm = MoveGetTest(mappos.x, mappos.y);
    if(!smm) return -1;
    int zone = smm->m_Zone;
    if(zone < 0) return -1;

    PrepareBuf();

    m_ZoneIndex[cnt] = zone;
    m_ZoneDataZero[zone] = 1;
    ++cnt;

    while(sme < cnt)
    {
        int zone_index = m_ZoneIndex[sme];
        if(m_RoadNetwork.m_Zone[zone_index].m_PlaceCnt > 0)
        {
            int place = m_RoadNetwork.m_Zone[zone_index].m_Place[0];
            int map_dist = mappos.Dist2(m_RoadNetwork.m_Place[place].m_Pos);
            for(int i = 1; i < m_RoadNetwork.m_Zone[zone_index].m_PlaceCnt; ++i)
            {
                int place2 = m_RoadNetwork.m_Zone[zone_index].m_Place[i];
                int map_dist2 = mappos.Dist2(m_RoadNetwork.m_Place[place2].m_Pos);
                if(map_dist2 < map_dist)
                {
                    map_dist = map_dist2;
                    place = place2;
                }
            }

            for(int i = 0; i < cnt; ++i) m_ZoneDataZero[m_ZoneIndex[i]] = 0;
            return place;
        }
        else
        {
            for(int i = 0; i < m_RoadNetwork.m_Zone[zone_index].m_NearZoneCnt; ++i)
            {
                int nz = m_RoadNetwork.m_Zone[zone_index].m_NearZone[i];
                if(m_ZoneDataZero[nz]) continue;
                if(m_RoadNetwork.m_Zone[zone_index].m_NearZoneMove[i] & mm) continue;

                m_ZoneIndex[cnt] = nz;
                m_ZoneDataZero[nz] = 1;
                ++cnt;
            }
        }

        ++sme;
    }

    for(int i = 0; i < cnt; ++i) m_ZoneDataZero[m_ZoneIndex[i]] = 0;
    return -1;
}

int CMatrixMapLogic::FindPlace(const CPoint& mappos)
{
    SMatrixMapMove* mm = MoveGetTest(mappos.x, mappos.y);
    if(!mm) return -1;
    int zone = mm->m_Zone;
    if(zone < 0) return -1;

    for(int i = 0; i < m_RoadNetwork.m_Zone[zone].m_PlaceCnt; ++i)
    {
        int pl = m_RoadNetwork.m_Zone[zone].m_Place[i];
        CPoint tp = m_RoadNetwork.m_Place[pl].m_Pos;

        if(mappos.x >= tp.x && mappos.x < (tp.y + 4) && mappos.y >= tp.y && mappos.y < (tp.y + 4)) return pl;

    }
    return -1;
}

int CMatrixMapLogic::PlaceList(byte mm, const CPoint &from, const CPoint &to, int radius, bool farpath, int *list, int *listcnt, int *outdist)
{
    int i, u, cnt, sme, np, dist, next, x, y, clcnt, oldcnt;
    SMatrixPlace* place;
    int findend = -1;

    *listcnt = 0;

    PrepareBuf();

    int fromplace = FindNearPlace(mm, from);
    if(fromplace < 0) return 0;

    int toplace = FindNearPlace(mm, to);

    if(toplace >= 0 && ((m_RoadNetwork.GetPlace(toplace)->m_Move & (1 + 2 + 4 + 8 + 16)) == (1 + 2 + 4 + 8 + 16) || !(m_RoadNetwork.GetPlace(toplace)->m_Move & mm)))
    {
        if(*listcnt >= m_RoadNetwork.m_PlaceCnt) __asm int 3;

        cnt = 0;
        sme = 0;
        m_ZoneIndex[cnt] = toplace;
        ++cnt;
        m_ZoneDataZero[toplace] = 1;
        list[*listcnt] = toplace;
        ++(*listcnt);
        if(fromplace == toplace) findend = 0;

        // ищем места в заданном радиусе
        while(sme < cnt)
        {
            place = m_RoadNetwork.GetPlace(m_ZoneIndex[sme]);
            for(i = 0; i < place->m_NearCnt; ++i)
            {
                np = place->m_Near[i];
                if(m_ZoneDataZero[np]) continue;
                if(place->m_NearMove[i] & mm) continue;
                if(to.Dist2(m_RoadNetwork.m_Place[np].m_Pos) > POW2(radius)) continue;

                if(*listcnt >= m_RoadNetwork.m_PlaceCnt) __asm int 3;

                if(fromplace == np) findend = 0;

                m_ZoneIndex[cnt] = np;
                ++cnt;
                m_ZoneDataZero[np] = 1;
                list[*listcnt] = np;
                ++(*listcnt);
            }

            ++sme;
        }

        //Можем ли дойти до начальной точки
        sme = 0;
        dist = 0;
        next = cnt;
        if(findend < 0)
        {
            while(sme < cnt)
            {
                place = m_RoadNetwork.GetPlace(m_ZoneIndex[sme]);
                for(i = 0; i < place->m_NearCnt; ++i)
                {
                    np = place->m_Near[i];
                    if(m_ZoneDataZero[np]) continue;
                    if(place->m_NearMove[i] & mm) continue;

                    if(np == fromplace) break;

                    if(cnt >= m_RoadNetwork.m_PlaceCnt) break;

                    m_ZoneIndex[cnt] = np;
                    ++cnt;
                    m_ZoneDataZero[np] = 1;
                }

                if(i < place->m_NearCnt) break;
                ++sme;

                if(sme >= next)
                {
                    next = cnt;
                    dist++;
                }
            }
        }
        if(outdist) *outdist = dist * 16;
        for(i = 0; i < cnt; ++i) m_ZoneDataZero[m_ZoneIndex[i]] = 0;
        if(sme < cnt)
        {
            if(farpath) return 1;
            else if (POW2(dist * (16 / 4)) <= from.Dist2(to)) return 1;
        }
    }

    *listcnt = 0;
    cnt = 0;
    sme = 0;

    // ищем места в заданном радиусе
    CRect rc;
    rc.left = to.x - radius - ROBOT_MOVECELLS_PER_SIZE;
    rc.top = to.y - radius - ROBOT_MOVECELLS_PER_SIZE;
    rc.right = to.x + radius + ROBOT_MOVECELLS_PER_SIZE;
    rc.bottom = to.y + radius + ROBOT_MOVECELLS_PER_SIZE;
    CRect plr = g_MatrixMap->m_RoadNetwork.CorrectRectPL(rc);

    SMatrixPlaceList *plist = g_MatrixMap->m_RoadNetwork.m_PLList + plr.left + plr.top * g_MatrixMap->m_RoadNetwork.m_PLSizeX;
    for(y = plr.top; y < plr.bottom; ++y, plist += g_MatrixMap->m_RoadNetwork.m_PLSizeX - (plr.right - plr.left))
    {
        for(x = plr.left; x < plr.right; ++x, plist)
        {
            place = g_MatrixMap->m_RoadNetwork.m_Place + plist->m_Sme;
            for(u = 0; u < plist->m_Cnt; ++u, ++place)
            {
                if(place->m_Move & mm) continue;
                if(to.Dist2(place->m_Pos) > POW2(radius)) continue;

                if(*listcnt >= m_RoadNetwork.m_PlaceCnt) __asm int 3;

                m_ZoneIndex[cnt] = plist->m_Sme + u;
                ++cnt;
                m_ZoneDataZero[plist->m_Sme + u] = (*listcnt + 1) | 0x80000000;
                list[*listcnt] = plist->m_Sme + u;
                ++(*listcnt);
            }
        }
    }

    //Кластеризуем
    findend = -1;
    oldcnt = cnt;
    clcnt = 0;
    for(u = 0; u < *listcnt; ++u)
    {
        if(m_ZoneDataZero[list[u]] & 0x7fff0000) continue;

        ++clcnt;

        sme = cnt;
        m_ZoneIndex[cnt] = list[u];
        ++cnt;
        m_ZoneDataZero[list[u]] |= clcnt << 16;

        next = cnt;

        while(sme < cnt)
        {
            place = m_RoadNetwork.GetPlace(m_ZoneIndex[sme]);
            for(i = 0; i < place->m_NearCnt; ++i)
            {
                np = place->m_Near[i];
                if(m_ZoneDataZero[np] & 0x7fff0000) continue;
                if(place->m_NearMove[i] & mm) continue;

                if(cnt >= m_RoadNetwork.m_PlaceCnt) break;

                if(m_ZoneDataZero[np] & 0x80000000)
                {
                    m_ZoneIndex[cnt] = np;
                    ++cnt;
                    m_ZoneDataZero[np] = m_ZoneDataZero[np] | (clcnt << 16);

                    if(fromplace == np) findend = m_ZoneDataZero[np] & 0x7fff0000;
                }
                else
                {
                    x = m_ZoneDataZero[m_ZoneIndex[sme]];
                    if(x & 0x80000000) x = 1;
                    else x = (x & 0xffff) + 1;
                    if(x < 4)
                    {
                        m_ZoneIndex[cnt] = np;
                        ++cnt;
                        m_ZoneDataZero[np] = x | (clcnt << 16);
                    }
                }
            }
            ++sme;
        }
    }
    for(i = oldcnt; i < cnt; ++i)
    {
        if(!(m_ZoneDataZero[m_ZoneIndex[i]] & 0x80000000)) m_ZoneDataZero[m_ZoneIndex[i]] = 0;
    }
    cnt = oldcnt;

    // ищем путь от кластеров до начальной точки
    sme = 0;
    dist = 0;

    next = cnt;
    u = 0;
    if(findend >= 0) u = findend;
    else
    {
        while(sme < cnt)
        {
            if(m_ZoneIndex[sme] == fromplace)
            {
                u = m_ZoneDataZero[m_ZoneIndex[sme]] & 0x7fff0000;
                break;
            }
            place = m_RoadNetwork.GetPlace(m_ZoneIndex[sme]);
            for(i = 0; i < place->m_NearCnt; ++i)
            {
                np = place->m_Near[i];
                if(m_ZoneDataZero[np] & 0x7fff0000) continue;
                if(place->m_NearMove[i] & mm) continue;

                if(np == fromplace)
                {
                    u = m_ZoneDataZero[m_ZoneIndex[sme]] & 0x7fff0000;
                    break;
                }

                if(cnt >= m_RoadNetwork.m_PlaceCnt) break;

                m_ZoneIndex[cnt] = np;
                ++cnt;
                m_ZoneDataZero[np] = m_ZoneDataZero[m_ZoneIndex[sme]] & 0x7fff0000;
            }

            if(u) break;
            ++sme;
            if(sme >= next)
            {
                next = cnt;
                ++dist;
            }
        }
    }

    if(!u)
    {
        for(i = 0; i < cnt; ++i) m_ZoneDataZero[m_ZoneIndex[i]] = 0;
        return 0;
    }

    if(outdist) *outdist = dist * 16;

    //Создаем конечный список
    *listcnt = 0;
    for(i = 0; i < oldcnt; ++i)
    {
        if((m_ZoneDataZero[m_ZoneIndex[i]] & 0x7fff0000) == u)
        {
            if(*listcnt >= m_RoadNetwork.m_PlaceCnt) __asm int 3;

            list[*listcnt] = m_ZoneIndex[i];
            ++(*listcnt);
        }
    }

    for(i = 0; i < cnt; ++i) m_ZoneDataZero[m_ZoneIndex[i]] = 0;
    return 2;
}

int CMatrixMapLogic::PlaceListGrow(byte mm, int *list, int *listcnt, int growcnt)
{
    int addcnt = 0;
    int i, np;
    int sme = 0;
    int cnt = 0;
    SMatrixPlace *place;

    PrepareBuf();

    for (i = 0; i < *listcnt; ++i)
    {
        m_ZoneIndex[cnt] = list[i];
        ++cnt;
        m_ZoneDataZero[list[i]] = 1;
    }

    while(sme < cnt)
    {
        place = m_RoadNetwork.GetPlace(m_ZoneIndex[sme]);
        for(i = 0; i < place->m_NearCnt; ++i)
        {
            np = place->m_Near[i];
            if(m_ZoneDataZero[np]) continue;
            if(place->m_NearMove[i] & mm) continue;

            if(*listcnt >= m_RoadNetwork.m_PlaceCnt) __asm int 3;

            m_ZoneIndex[cnt] = np;
            ++cnt;
            m_ZoneDataZero[np] = 1;
            list[*listcnt] = np;
            ++(*listcnt);

            addcnt++;
            if(addcnt >= growcnt) break;
        }
        if(i < place->m_NearCnt) break;

        ++sme;
    }

    for(i = 0; i < cnt; ++i) m_ZoneDataZero[m_ZoneIndex[i]] = 0;
    return addcnt;
}

bool takt_effect(dword key, dword val, dword user);


float SQ(float x)
{
    return (float)sqrt(x);
}

float LNSQ(const D3DXVECTOR3 &v)
{
    return D3DXVec3LengthSq(&v);
}

float LN(const D3DXVECTOR3 &v)
{
    return D3DXVec3Length(&v);
}

float NORM(D3DXVECTOR3 &vo, const D3DXVECTOR3 &v)
{
    D3DXVec3Normalize(&vo, &v);
    return vo.y;
}

//CWStr hhh(const CWStr &tex)
//{
//    if(tex == L"bla") return CWStr(L"yo!");
//    return CWStr(L"---");
//}

static bool Egg1(const D3DXVECTOR2& center, CMatrixMapStatic* ms, dword user)
{
    int* egg = (int*)user;
    if(ms->IsRobotAlive())
    {
        CMatrixRobotAI* robot = ms->AsRobot();

        ++(*egg);

        if(ms->GetSide() != PLAYER_SIDE) (*egg) += 100;
        if((!robot->HaveBomb() || fabs(robot->GetMaxHitpoints() - robot->GetHitpoints()) > 1.0f) && !robot->IsInPosition())
        {
            (*egg) += 100;
        }

        if(*egg == 1) robot->MarkInPosition();
    }
    return true;
}

static bool Egg2(const D3DXVECTOR2& center, CMatrixMapStatic* ms, dword user)
{
    int* egg = (int*)user;
    if(ms->IsRobotAlive())
    {
        ++(*egg);
    }
    return true;
}

void CMatrixMapLogic::Tact(int step)
{
    if(g_RangersInterface)
    {
        g_RangersInterface->m_Tact();

        if(FLAG(m_Flags, MMFLAG_MUSIC_VOL_CHANGING))
        {
            float cv = g_RangersInterface->m_MusicVolumeGet();
            if(fabs(cv-m_TargetMusicVolume) < 0.001f)
            {
                RESETFLAG(m_Flags, MMFLAG_MUSIC_VOL_CHANGING);
                g_RangersInterface->m_MusicVolumeSet(m_TargetMusicVolume);
            }
            else
            {
                float delta = 0;
                if(cv < m_TargetMusicVolume) delta = 1;
                else delta = -1;
                g_RangersInterface->m_MusicVolumeSet(cv + delta * float(step) * 0.001f);
            }
        }
    }

    //Передаём команду функции для отрисовки точки сборного пункта здания
    if(m_GatheringPointsList.BuildingsList.size())
    {
        for(int i = 0; i < m_GatheringPointsList.BuildingsList.size(); ++i)
        {
            CMatrixBuilding* bld = m_GatheringPointsList.BuildingsList[i];
            bld->ShowGatheringPointTact(GetTime());
        }
    }

    if(FLAG(g_MatrixMap->m_Flags, MMFLAG_STAT_DIALOG | MMFLAG_STAT_DIALOG_D | MMFLAG_MUSIC_VOL_CHANGING | MMFLAG_TRANSITION))
    {
        // some modes...

        //_________________________________________________________________________________________________

        if(FLAG(m_Flags, MMFLAG_TRANSITION))
        {
            m_Transition.Tact(step);
        }

        //_________________________________________________________________________________________________


        if(FLAG(m_Flags, MMFLAG_MUSIC_VOL_CHANGING))
        {
            float cv = g_RangersInterface->m_MusicVolumeGet();
            if(fabs(cv-m_TargetMusicVolume) < 0.001f)
            {
                RESETFLAG(m_Flags, MMFLAG_MUSIC_VOL_CHANGING);
                g_RangersInterface->m_MusicVolumeSet(m_TargetMusicVolume);
            }
            else
            {
                float delta = 0;
                if(cv < m_TargetMusicVolume) delta = 1;
                else delta = -1;
                g_RangersInterface->m_MusicVolumeSet(cv + delta * float(step) * 0.001f);
            }

        }

        //_________________________________________________________________________________________________


        if(FLAG(g_MatrixMap->m_Flags, MMFLAG_STAT_DIALOG | MMFLAG_STAT_DIALOG_D))
        {
            // stat
            CBlockPar* repl = g_MatrixData->BlockGet(PAR_REPLACE);
            CWStr temp(g_CacheHeap);
            for(int i = 0; i < m_SidesCount; ++i)
            {
                CMatrixSideUnit* su = m_Side + i;

                if(su == GetPlayerSide()) temp = L"_p_";
                else
                {
                    temp.Set(g_MatrixData->BlockGet(L"Side")->ParGet(CWStr(su->m_Id, g_CacheHeap))[0]);
                    temp.LowerCase();
                    temp.Insert(0, L"_");
                    temp += L"_";
                }

                if(!su->GetStatValue(STAT_TIME))
                {
                    repl->ParSetAdd(temp + L"c6", L"");
                }
                else
                {
                    bool winer = false;
                    int time = su->GetStatValue(STAT_TIME) / 1000;
                    if(time < 0)
                    {
                        time = -time;
                        winer = true;
                    }
                    int hours = time / 3600;
                    int mins = (time - hours * 3600) / 60;
                    int secs = (time - hours * 3600 - mins * 60);

                    CWStr h(hours,g_CacheHeap);
                    CWStr m(mins,g_CacheHeap);
                    CWStr s(secs,g_CacheHeap);
                    if(h.GetLen() < 2) h.Insert(0, L"0");
                    if(m.GetLen() < 2) m.Insert(0, L"0");
                    if(s.GetLen() < 2) s.Insert(0, L"0");

                    if(winer)
                    {
                        repl->ParSetAdd(temp + L"c6", L"<color=0,255,0>" + h + L":" + m + L":" + s + L"</color>");
                    }
                    else
                    {
                        repl->ParSetAdd(temp + L"c6", h + L":" + m + L":" + s);
                    }
                
                    repl->ParSetAdd(temp + L"c1", CWStr(su->GetStatValue(STAT_ROBOT_BUILD), g_CacheHeap));
                    repl->ParSetAdd(temp + L"c2", CWStr(su->GetStatValue(STAT_ROBOT_KILL), g_CacheHeap));
                    repl->ParSetAdd(temp + L"c3", CWStr(su->GetStatValue(STAT_TURRET_BUILD), g_CacheHeap));
                    repl->ParSetAdd(temp + L"c4", CWStr(su->GetStatValue(STAT_TURRET_KILL), g_CacheHeap));
                    repl->ParSetAdd(temp + L"c5", CWStr(su->GetStatValue(STAT_BUILDING_KILL), g_CacheHeap));
                }
            }

            if(FLAG(m_Flags, MMFLAG_STAT_DIALOG)) EnterDialogMode(TEMPLATE_DIALOG_STATISTICS);
            else EnterDialogMode(TEMPLATE_DIALOG_STATISTICS_D);

            RESETFLAG(m_Flags, MMFLAG_STAT_DIALOG | MMFLAG_STAT_DIALOG_D);
        }
    }

    CMultiSelection::AddTime(step);


    if(m_Console.IsActive())
    {
        m_Console.Tact(step);
    }

    if(IsPaused()) 
    {
        if(m_GameSpeedHint)
        {
            m_GameSpeedHint->Release();
            m_GameSpeedHint = nullptr;
        }

        if(m_PauseHint == nullptr && g_RangersInterface && !FLAG(m_Flags, MMFLAG_DIALOG_MODE))
        {
            m_PauseHint = CMatrixHint::Build(CWStr(TEMPLATE_PAUSE));
            m_PauseHint->Show(14, 62);
        }

        g_IFaceList->LogicTact(step);

        m_Cursor.Tact(step);
        m_DI.Tact(step);

        CMatrixMapStatic* ms;
        ms = CMatrixMapStatic::GetFirstLogic();
	    while(ms)
        {
            if(ms->GetObjectType() == OBJECT_TYPE_MAPOBJECT) ((CMatrixMapObject*)ms)->PauseTact(step);
            else if(ms->IsBuildingAlive()) ms->AsBuilding()->PauseTact(step);
            else if(ms->IsTurretAlive()) ms->AsTurret()->PauseTact(step);
            else if(ms->IsRobotAlive()) ms->AsRobot()->PauseTact(step);
            ms = ms->GetNextLogic();
	    }

	    int cnt = m_VisibleGroupsCount;
        CMatrixMapGroup **md = m_VisibleGroups;
	    while(--cnt > 0)
        {
            (*(++md))->PauseTact(step);
	    }

        CMatrixMapStatic::CalcDistances();

        float fstep = (float)step;
        m_Minimap.PauseTact(fstep);
        m_Camera.Tact(fstep);
        return;
    }
    else
    {
        if(m_PauseHint)
        {
            m_PauseHint->Release();
            m_PauseHint = nullptr;
        }

        if(g_GameSpeedFactor != 1.0f && (!m_GameSpeedHint || (m_GameSpeedHintVal != g_GameSpeedFactor)))
        {
            if(m_GameSpeedHint)
            {
                m_GameSpeedHint->Release();
                m_GameSpeedHint = nullptr;
            }

            m_GameSpeedHint = CMatrixHint::Build(CWStr(TEMPLATE_GAME_SPEED), L"", CWStr(L" " + CWStr(int(g_GameSpeedFactor * 100.0f)) + L"%"));
            m_GameSpeedHint->Show(14, 62);

            m_GameSpeedHintVal = g_GameSpeedFactor;
        }
        else if(m_GameSpeedHint && g_GameSpeedFactor == 1.0f)
        {
            m_GameSpeedHint->Release();
            m_GameSpeedHint = nullptr;

            m_GameSpeedHintVal = g_GameSpeedFactor;
        }
    }

    m_Time += step;
    if(m_ReinforcementsTime > 0)
    {
        m_ReinforcementsTime -= step;
        if(m_ReinforcementsTime < 0)
        {
            CSound::Play(S_REINFORCEMENTS_READY, SL_INTERFACE);
            m_ReinforcementsTime = 0;
        }
    }

    /*
    if(m_ShadeOn)
    {
        m_ShadeTime -= step;
        m_ShadeTimeCurrent += step;
        if(m_ShadeTime < 0)
        {
            m_ShadeTime = 0;
        }

        float k = (float(m_ShadeTime)/SHADER_TIME);
        m_Minimap.SetColor(LIC(0x00000000, m_ShadeInterfaceColor, k));

        double xx1 = (0.05 * double(m_ShadeTimeCurrent));
        double xx2 = (double(m_ShadeTimeCurrent) - 1500.0);
        float speed = float((xx1 * xx1 + xx2 * xx2) / 2000000.0);

        float nt =  float(step) * speed;

        m_Camera.RotateZ(GRAD2RAD(step * 0.05f));
        
        if(m_ShadeTimeCurrent > 1000 && m_ShadeTimeCurrent < 5000)
        {
            m_Camera.RotateX(GRAD2RAD(-step * 0.00001f * (m_ShadeTimeCurrent - 1000)));
            //m_Camera.RotateZ(step * 0.001f * (m_ShadeTimeCurrent - 1000));
        }
        //else {}

        for(int i = 0; i < m_EffectsCnt; ++i) m_Effects[i]->Tact(nt);

        return;
    }
    */

    // TODO : 10 time per second
	g_IFaceList->LogicTact(step);

    if((GetTime() - m_GatherInfoLast) > 100)
    {
        m_GatherInfoLast = GetTime();

        GatherInfo(0);
        GatherInfo(1);
        //GatherInfo(2);
    }

    int portions = step / int(LOGIC_TACT_DIVIDER * g_GameSpeedFactor);
    
    for(int cnt = 0; cnt < portions; ++cnt)
    {
        CMatrixMapStatic::ProceedLogic(int(LOGIC_TACT_DIVIDER * g_GameSpeedFactor));
    }

    portions = step - portions * int(LOGIC_TACT_DIVIDER * g_GameSpeedFactor);
    if(portions)
    {
        CMatrixMapStatic::ProceedLogic(portions);
    }

    while(GetTime() > m_TactNext)
    {
		m_TactNext += int(LOGIC_TACT_DIVIDER * g_GameSpeedFactor);
        //CMatrixMapStatic::ProceedLogic(LOGIC_TACT_DIVIDER * g_GameSpeedFactor);

		for(int i = 0; i < m_SidesCount; ++i)
        {
			m_Side[i].LogicTact(int(LOGIC_TACT_DIVIDER * g_GameSpeedFactor));
		}
	}

    if(GetPlayerSide()->GetUnitUnderManualControl())
    {
        GetPlayerSide()->GetUnitUnderManualControl()->StaticTact(step);
        if(GetPlayerSide()->GetUnitUnderManualControl())
        {
            GetPlayerSide()->GetUnitUnderManualControl()->GetResources(MR_Matrix);
            //Используется для последующего выравнивания положения корпуса в ручном режиме управления
            GetPlayerSide()->InterpolateManualControlledRobotHullPos(step);
        }
    }

    CMatrixMap::Tact(step); // graphic tacts after logic tact
    m_Camera.Tact(float(step));

    // check side status

    if((GetTime() - m_PrevTimeCheckStatus) > 1001)
    {
        // check easter egg :)
        CWStr mn(MapName(), g_CacheHeap);
        mn.LowerCase();
        if(mn.Find(L"terron") >= 0)
        {
            int egg1 = 0, egg2 = 0;
            FindObjects(D3DXVECTOR2(3833.8f, 2298.1f), 50, 1, TRACE_ROBOT, nullptr, Egg1, (dword)&egg1);
            if(egg1 == 1)
            {
                FindObjects(D3DXVECTOR2(3833.8f, 2298.1f), 160, 1, TRACE_ROBOT, nullptr, Egg2, (dword)&egg2);
                if(egg2 == 1)
                {
                    SETFLAG(g_MatrixMap->m_Flags, MMFLAG_ROBOT_IN_POSITION);
                }
                else
                {
                    goto oblom;
                }
            }
            else
            {
oblom:
                RESETFLAG(g_MatrixMap->m_Flags, MMFLAG_ROBOT_IN_POSITION);
                CMatrixMapStatic *ms = CMatrixMapStatic::GetFirstLogic();
                for(; ms; ms = ms->GetNextLogic())
                {
                    if(ms->IsRobotAlive()) ms->AsRobot()->UnMarkInPosition();
                }
            }
        }

        // update material ones per second
	    D3DMATERIAL9 mtrl;
	    ZeroMemory(&mtrl,sizeof(D3DMATERIAL9));
	    mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
	    mtrl.Diffuse.g = mtrl.Ambient.g = 1.0f;
	    mtrl.Diffuse.b = mtrl.Ambient.b = 1.0f;
	    mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
	    mtrl.Specular.r = 0.5f;
	    mtrl.Specular.g = 0.5f;
	    mtrl.Specular.b = 0.5f;
	    mtrl.Specular.a = 0.5f;
	    g_D3DD->SetMaterial(&mtrl );


        // check only one time per second
        m_PrevTimeCheckStatus = GetTime() + 1001;

        int cnt = 0;
        for(int i = 0; i < m_SidesCount; ++i)
        {
            if (m_Side[i].m_Id >= cnt) cnt = m_Side[i].m_Id + 1;
        }
        ASSERT(cnt < 1000);

        int sz = cnt;
        bool* sides = (bool*)_malloca(sz);
        if(!sides) ERROR_S(L"Not enough memory for sides after _malloca() call in CMatrixMapLogic::Tact function!");
        bool* sides2 = (bool*)_malloca(sz);
        if(!sides2) ERROR_S(L"Not enough memory for sides2 after _malloca() call in CMatrixMapLogic::Tact function!");
        memset(sides, 0, sz);
        memset(sides2, 0, sz);
        sides[0] = true;

        int scnt = 0;
        for(int i = 0; i < m_SidesCount; ++i)
        {
            if(m_Side[i].GetStatus() == SS_NONE)
            {
                sides2[m_Side[i].m_Id] = true;
                sides[m_Side[i].m_Id] = true;
                ++scnt;
            }
        }

        CMatrixMapStatic* ms = CMatrixMapStatic::GetFirstLogic();
        for(; ms; ms = ms->GetNextLogic())
        {
            int side = ms->GetSide();
            if(sides[side]) continue;
            if(ms->IsRobotAlive() || (ms->IsBase() && ms->IsBuildingAlive()))
            {
                sides[side] = true;
                ++scnt;
                if(scnt == m_SidesCount) break;
            }
        }

        for(int i = 0; i < m_SidesCount; ++i)
        {
            if(!sides[m_Side[i].m_Id])
            {
                //Ignore player dead if terron dead
                if(FLAG(g_MatrixMap->m_Flags, MMFLAG_TERRON_DEAD) && (m_Side+i) == g_MatrixMap->GetPlayerSide()) continue;
                //Ignore player dead if all specials broken
                if(FLAG(g_MatrixMap->m_Flags, MMFLAG_SPECIAL_BROKEN) && (m_Side + i) == g_MatrixMap->GetPlayerSide()) continue;

                m_Side[i].SetStatus(SS_JUST_DEAD);

                CMatrixMapStatic *ms = CMatrixMapStatic::GetFirstLogic();
                for(; ms; ms = ms->GetNextLogic())
                {
                    if(ms->IsBuildingAlive())
                    {
                        int side = ms->GetSide();
                        if(side == m_Side[i].m_Id)
                        {
                            ms->AsBuilding()->SetNeutral();
                        }
                    }
                }
            }
        }

        // checking win/lose status

        if(m_BeforeWinLoseDialogCount > 1) --m_BeforeWinLoseDialogCount; 
        else if(m_BeforeWinLoseDialogCount == 1)
        {
            //Если был активен режим автобоя, то не побеждает никто
            if(FLAG(g_MatrixMap->m_Flags, MMFLAG_FULLAUTO))
            {
                g_ExitState = ES_EXIT_TO_MAIN_MENU;
                SETFLAG(g_Flags, GFLAG_EXITLOOP);
            }
            else
            {
                //Если игрок победил
                if(FLAG(m_Flags, MMFLAG_WIN))
                {
                    GetPlayerSide()->SetStatValue(STAT_TIME, -GetPlayerSide()->GetStatValue(STAT_TIME));
                    EnterDialogMode(TEMPLATE_DIALOG_WIN);
                }
                //Если игрок проиграл
                else EnterDialogMode(TEMPLATE_DIALOG_LOSE);
            }
        }
        else
        {
            if(GetPlayerSide()->GetStatus() == SS_JUST_WIN)
            {
                GetPlayerSide()->SetStatus(SS_ACTIVE);

                m_BeforeWinLoseDialogCount = 1;
                SETFLAG(m_Flags, MMFLAG_WIN);
            }
            else if(GetPlayerSide()->GetStatus() == SS_JUST_DEAD)
            {
                for(int i = 0; i < m_SidesCount; ++i)
                {
                    if(sides[m_Side[i].m_Id] && !sides2[m_Side[i].m_Id])
                    {
                        m_Side[i].SetStatValue(STAT_TIME, -m_Side[i].GetStatValue(STAT_TIME));
                    }
                }

                GetPlayerSide()->SetStatus(SS_NONE);
                m_BeforeWinLoseDialogCount = 1;
                RESETFLAG(m_Flags, MMFLAG_WIN);
            }
            else
            {
                int acnt = 0;
                for(int i = 0; i < m_SidesCount; ++i)
                {
                    if(m_Side[i].GetStatus() == SS_JUST_DEAD)
                    {
                        m_Side[i].SetStatus(SS_NONE);
                    }
                    if(m_Side[i].GetStatus() == SS_ACTIVE)
                    {
                        ++acnt;
                    }
                }

                if(acnt == 1 && g_MatrixMap->m_BeforeWinCount <= 0)
                {
                    // only one side is active
                    // it is player side. win!
                    m_BeforeWinLoseDialogCount = 1;
                    SETFLAG(m_Flags, MMFLAG_WIN);
                }
            }
        }

        _freea(sides); //Очистка после вызова _malloca
        _freea(sides2);
    }
}

SMatrixPathObj* CMatrixMapLogic::ObjAdd()
{
    SMatrixPathObj* obj = (SMatrixPathObj*)HAllocClear(sizeof(SMatrixPathObj), Base::g_MatrixHeap);
    LIST_ADD(obj, m_ObjFirst, m_ObjLast, m_Prev, m_Next);
    return obj;
}

void CMatrixMapLogic::ObjDelete(SMatrixPathObj* obj)
{
    LIST_DEL(obj, m_ObjFirst, m_ObjLast, m_Prev, m_Next);
    HFree(obj, Base::g_MatrixHeap);
}

void CMatrixMapLogic::ObjClear()
{
    while(m_ObjFirst) ObjDelete(m_ObjLast);
}

SMatrixPath *CMatrixMapLogic::PathAlloc()
{
    return (SMatrixPath*)HAllocClear(sizeof(SMatrixPath), Base::g_MatrixHeap);
}

void CMatrixMapLogic::PathFree(SMatrixPath *path)
{
    HFree(path, Base::g_MatrixHeap);
}

void CMatrixMapLogic::PathClear()
{
    while(m_PathFirst) PathDelete(m_PathLast);
}

void CMatrixMapLogic::PathCalcPosAfter(SMatrixPath *path)
{
    SMatrixPathObj* obj = m_ObjFirst;
    while(obj)
    {
        obj->tx = obj->cx;
        obj->ty = obj->cy;
        obj->calc_pos = false;
        obj = obj->m_Next;
    }

    while(path)
    {
        if(!path->m_Owner->calc_pos)
        {
            path->m_Owner->tx=path->m_Module[path->m_Cnt-1].ex;
            path->m_Owner->ty=path->m_Module[path->m_Cnt-1].ey;
            path->m_Owner->calc_pos=true;
        }
        path=path->m_Prev;
    }
}

bool CMatrixMapLogic::PathCheckInFindInsert(SMatrixPath* path)
{
    SMatrixPath* afterpath = m_PathLast;
    while(afterpath)
    {
        if(afterpath->m_Owner->m_Path == afterpath) break;
        afterpath = afterpath->m_Prev;
    }
    PathCalcPosAfter(afterpath);

    if(!PathIntersectAfter(path))
    {
        if(path == nullptr) PathInsert(m_PathFirst, path);
        else PathInsert(afterpath->m_Next, path);
        return true;
    }
    if(afterpath == nullptr) afterpath = m_PathFirst;
    else afterpath = afterpath->m_Next;

    while(afterpath)
    {
        afterpath->m_Owner->tx = afterpath->m_Module[afterpath->m_Cnt - 1].ex;
        afterpath->m_Owner->ty = afterpath->m_Module[afterpath->m_Cnt - 1].ey;

        if(!PathIntersectAfter(path))
        {
            PathInsert(afterpath, path);
            return true;
        }
        afterpath = afterpath->m_Next;
    }
    return false;
}

//Нигде не используется и ничего не делает
/*
void CMatrixMapLogic::PathCheckIn(SMatrixPath *path)
{
    if(PathCheckInFindInsert(path)) return;

    // CRect re = CRect(Float2Int(path->m_StartX - path->m_Owner->radius), Float2Int(path->m_StartY - path->m_Owner->radius), Float2Int(path->m_EndX + path->m_Owner->radius), Float2Int(path->m_EndY + path->m_Owner->radius));
    // int size = 16;
    // re.left = max(re.left - size, 0);
    // re.top = max(re.top - size, 0);
    // re.right = min(re.right + size, m_SizeMove.x - size);
    // re.bottom = min(re.bottom + size, m_SizeMove.y - size);

    // SMatrixMapMove *smm = MoveGet(re.left, re.top);
    // for(y = re.top; y < re.bottom; ++y, smm += m_SizeMove.x - (re.right - re.left))
    // {
        // for(x = re.left; x < re.right; ++x, ++smm)
        // {
            // smm->m_Find = -1;
        // }
    // }

    PathCalcPosAfter(m_PathLast);
    SMatrixPathObj *obj = m_ObjFirst;
    while(obj)
    {
        if(obj != path->m_Owner)
        {
            if(PathIntersect(path, obj->tx, obj->ty, obj->radius))
            {
                if(PathCheckInFindPos(path, obj)) {}
            }
        }
        obj = obj->m_Next;
    }
}
*/

bool CMatrixMapLogic::PathCheckInFindPos(SMatrixPath *path, SMatrixPathObj *obj)
{
    int mx = Float2Int(obj->tx / GLOBAL_SCALE_MOVE);
    int my = Float2Int(obj->ty / GLOBAL_SCALE_MOVE);

    int tx, ty, u, i = 0;
    while(i < m_SizeMove.x)
    {
        for(u = 0; u < ((i + 1) * 2 + 1); ++u)
        {
            tx = mx - (i + 1) + u;
            ty = my - (i + 1);
            if(IsAbsenceWall(obj->nsh, obj->size, tx, ty))
            {
                if(!PathIntersect(path, obj->tx, obj->ty, obj->radius))
                {
                    obj->tx = GLOBAL_SCALE_MOVE * tx; obj->ty = GLOBAL_SCALE_MOVE * ty;
                    return true;
                }
            }
            tx = mx - (i + 1) + u;
            ty = my + (i + 1);
            if(IsAbsenceWall(path->m_Owner->nsh, path->m_Owner->size, tx, ty))
            {
                if(!PathIntersect(path, obj->tx, obj->ty, obj->radius))
                {
                    obj->tx = GLOBAL_SCALE_MOVE * tx; obj->ty = GLOBAL_SCALE_MOVE * ty;
                    return true;
                }
            }
        }

        for(u = 0; u < (i * 2 + 1); ++u)
        {
            tx = mx - (i + 1);
            ty = my - i + u;
            if(IsAbsenceWall(path->m_Owner->nsh, path->m_Owner->size, tx, ty))
            {
                if(!PathIntersect(path, obj->tx, obj->ty, obj->radius))
                {
                    obj->tx = GLOBAL_SCALE_MOVE * tx; obj->ty = GLOBAL_SCALE_MOVE * ty;
                    return true;
                }
            }
            tx = mx + (i + 1);
            ty = my - i + u;
            if(IsAbsenceWall(path->m_Owner->nsh, path->m_Owner->size, tx, ty))
            {
                if(!PathIntersect(path, obj->tx, obj->ty, obj->radius))
                {
                    obj->tx = GLOBAL_SCALE_MOVE * tx; obj->ty = GLOBAL_SCALE_MOVE * ty;
                    return true;
                }
            }
        }
        ++i;
    }
    return false;
}

bool CMatrixMapLogic::PathIntersectAfter(SMatrixPath *path)
{
    SMatrixPathObj *obj = m_ObjFirst;
    while (obj)
    {
        if(obj != path->m_Owner)
        {
            if (PathIntersect(path, obj->tx, obj->ty, obj->radius)) return true;
        }
        obj = obj->m_Next;
    }
    return false;
}

bool CMatrixMapLogic::PathIntersect(SMatrixPath *path, float cx, float cy, float radius)
{
    if(cx <= path->m_StartX - path->m_Owner->radius - radius || cx >= path->m_StartX + path->m_Owner->radius + radius ||
        cy <= path->m_StartY - path->m_Owner->radius - radius || cy >= path->m_StartY + path->m_Owner->radius + radius) return false;

    SMatrixPathUnit* pu = path->m_Module;
    for(int i = 0; i < path->m_Cnt; ++i, ++pu)
    {
        float vx = cx - pu->sx;
        float vy = cy - pu->sy;

        float dp = vx * pu->vx + vy * pu->vy;
        if(dp >= 0.0f && dp <= pu->length) //Сфера между точками
        {
            dp = -vx * pu->vy + vy * pu->vx;
            if(dp < POW2(radius + path->m_Owner->radius)) return true;
        }
        else
        {
            if((POW2(pu->sx - cx) + POW2(pu->sy - cy)) < POW2(radius + path->m_Owner->radius)) return true; // Пересекается начало
            if((POW2(pu->ex - cx) + POW2(pu->ey - cy)) < POW2(radius + path->m_Owner->radius)) return true; // Пересекается конец
        }
    }
    return false;
}

/*
bool CMatrixMapLogic::PathIntersect(SMatrixPath *path1, SMatrixPath *path2)
{
    int ox1=0; // Center
    if(path2->m_StartX<=path1->m_StartX-path1->m_Radius-path2->m_Radius) ox1=-1; // left
    else if(path2->m_StartX>=path1->m_EndX+path1->m_Radius+path2->m_Radius) ox1=1; // right

    int ox2=0; // Center
    if(path2->m_EndX<=path1->m_StartX-path1->m_Radius-path2->m_Radius) ox1=-1; // left
    else if(path2->m_EndX>=path1->m_EndX+path1->m_Radius+path2->m_Radius) ox1=1; // right

    if(ox1==ox2) return false;

    int oy1=0; // Center
    if(path2->m_StartY<=path1->m_StartY-path1->m_Radius-path2->m_Radius) oy1=-1; // left
    else if(path2->m_StartY>=path1->m_EndY+path1->m_Radius+path2->m_Radius) oy1=1; // right

    int oy2=0; // Center
    if(path2->m_EndY<=path1->m_StartY-path1->m_Radius-path2->m_Radius) oy1=-1; // left
    else if(path2->m_EndY>=path1->m_EndY+path1->m_Radius+path2->m_Radius) oy1=1; // right

    if(oy1==oy2) return false;
}
*/

void CMatrixMapLogic::CalcCannonPlace()
{
    CMatrixMapStatic* obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsTurret())
        {
            CMatrixTurret* turret = obj->AsTurret();
            turret->m_Place = m_RoadNetwork.FindInPL(CPoint(Float2Int(turret->m_Pos.x / GLOBAL_SCALE_MOVE), Float2Int(turret->m_Pos.y / GLOBAL_SCALE_MOVE)));
            if(turret->m_Place < 0) ERROR_S(L"CMatrixMapLogic::CalcCannonPlace Error!"); //Место не найдено
        }

        obj = obj->GetNextLogic();
    }
}

bool CMatrixMapLogic::IsLogicVisible(CMatrixMapStatic* ofrom, CMatrixMapStatic* oto, float second_z)
{
    D3DXVECTOR3 vt;
    D3DXVECTOR3 vstart = ofrom->GetGeoCenter();
    D3DXVECTOR3 vend;

    if(oto->IsTurret())
    {
        vend.x = ((CMatrixTurret*)(oto))->m_Pos.x;
        vend.y = ((CMatrixTurret*)(oto))->m_Pos.y;
        vend.z = g_MatrixMap->GetZ(vend.x, vend.y) + 20.0f;
    }
    else
    {
        vend = oto->GetGeoCenter();
    }

    while(true)
    {
        CMatrixMapStatic* trace_res = g_MatrixMap->Trace(&vt, vstart, vend, TRACE_ANYOBJECT | TRACE_NONOBJECT | TRACE_OBJECT_SPHERE | TRACE_SKIP_INVISIBLE, ofrom);
        if(IS_TRACE_STOP_OBJECT(trace_res) && trace_res->IsBuilding())
        {
            trace_res = g_MatrixMap->Trace(&vt, vstart, vend, TRACE_BUILDING | TRACE_SKIP_INVISIBLE, ofrom);
            if(IS_TRACE_STOP_OBJECT(trace_res) && trace_res->IsBuilding()) break;
            trace_res = g_MatrixMap->Trace(&vt, vstart, vend, (TRACE_OBJECT | TRACE_ROBOT | TRACE_FLYER | TRACE_TURRET) | TRACE_NONOBJECT | TRACE_OBJECT_SPHERE | TRACE_SKIP_INVISIBLE, ofrom);
        }
        if(IS_TRACE_STOP_OBJECT(trace_res) && trace_res == oto) return true;
        break;
    }
    if(second_z == 0.0f) return false;
    vstart.z += 50.0f;

    while(true)
    {
        CMatrixMapStatic *trace_res = g_MatrixMap->Trace(&vt, vstart, vend, TRACE_ANYOBJECT | TRACE_NONOBJECT | TRACE_OBJECT_SPHERE | TRACE_SKIP_INVISIBLE, ofrom);
        if(IS_TRACE_STOP_OBJECT(trace_res) && trace_res->IsBuilding())
        {
            trace_res = g_MatrixMap->Trace(&vt, vstart, vend, TRACE_BUILDING | TRACE_SKIP_INVISIBLE, ofrom);
            if(IS_TRACE_STOP_OBJECT(trace_res) && trace_res->IsBuilding()) break;
            trace_res = g_MatrixMap->Trace(&vt, vstart, vend, (TRACE_OBJECT | TRACE_ROBOT | TRACE_FLYER | TRACE_TURRET) | TRACE_NONOBJECT | TRACE_OBJECT_SPHERE | TRACE_SKIP_INVISIBLE, ofrom);
        }
        if(IS_TRACE_STOP_OBJECT(trace_res) && trace_res == oto) return true;
        break;
    }
    return false;
}

void CMatrixMapLogic::DumpLogic()
{
    FILE *fi;

    const char *ats[] = { "mlat_None","mlat_Defence","mlat_Attack","mlat_Forward","mlat_Retreat","mlat_Capture","mlat_Intercept" };
    const char *rss[] = { "ROBOT_IN_SPAWN","ROBOT_BASE_MOVEOUT","ROBOT_SUCCESSFULLY_BUILD","ROBOT_CARRYING","ROBOT_FALLING","ROBOT_DIP","ROBOT_CAPTURING_BASE","ROBOT_EMBRYO" };
    const char *ros[] = { "ROBOT_EMPTY_ORDER","ROT_MOVE_TO","ROT_MOVE_BACK","ROT_MOVE_RETURN","ROT_STOP_MOVE","ROT_FIRE","ROT_STOP_FIRE","ROT_CAPTURE_BUILDING","ROT_STOP_CAPTURE" };
    const char *rps[] = { "ROBOT_EMPTY_PHASE","ROBOT_WAITING_FOR_PARAMS","ROBOT_MOVING","ROBOT_FIRING","ROP_CAPTURE_MOVING","ROP_CAPTURE_IN_POSITION","ROP_CAPTURE_SETTING_UP","ROP_CAPTURING","ROBOT_GETING_LOST" };

    if((fi = fopen("#DumpLogic.txt", "w+")) == nullptr) return;

    for(int is = 0; is < m_SidesCount; ++is)
    {
        if(m_Side[is].GetRobotsCnt() <= 0) continue;
        fprintf(fi, "=== Side(%d)    Id(%d) ==========================================\n", is, m_Side[is].m_Id);
        fprintf(fi, "    m_RobotsCnt=%d\n", m_Side[is].GetRobotsCnt());

        fprintf(fi, "    m_TitanCnt=%d\n", m_Side[is].GetResourceAmount(TITAN));
        fprintf(fi, "    m_PlasmaCnt=%d\n", m_Side[is].GetResourceAmount(PLASMA));
        fprintf(fi, "    m_ElectronicCnt=%d\n", m_Side[is].GetResourceAmount(ELECTRONICS));
        fprintf(fi, "    m_EnergyCnt=%d\n", m_Side[is].GetResourceAmount(ENERGY));
        fprintf(fi, "\n");

        for(int it = 0; it < m_Side[is].m_TeamCnt; ++it)
        {
            if(m_Side[is].m_Team[it].m_RobotCnt <= 0) continue;
            fprintf(fi, "    --- Team(%d) ---\n", it);

            SMatrixTeam *team = m_Side[is].m_Team + it;

            fprintf(fi, "    m_GroupCnt=%d\n", team->m_GroupCnt);
            fprintf(fi, "    m_CenterMass=%d,%d\n", team->m_CenterMass.x, team->m_CenterMass.y);
            fprintf(fi, "    m_RadiusMass=%d\n", team->m_RadiusMass);
            fprintf(fi, "    m_Rect=%d,%d,%d,%d\n", team->m_Rect.left, team->m_Rect.top, team->m_Rect.right, team->m_Rect.bottom);
            fprintf(fi, "    m_Center=%d,%d\n", team->m_Center.x, team->m_Center.y);
            fprintf(fi, "    m_Radius=%d\n", team->m_Radius);
            fprintf(fi, "    m_RegionMassPrev=%d\n", team->m_RegionMassPrev);
            fprintf(fi, "    m_RegionMass=%d\n", team->m_RegionMass);
            fprintf(fi, "    m_RegionNearEnemy=%d\n", team->m_RegionNearEnemy);
            fprintf(fi, "    m_RegionNearRetreat=%d\n", team->m_RegionNearRetreat);
            fprintf(fi, "    m_RegionNearForward=%d\n", team->m_RegionNearForward);
            fprintf(fi, "    m_RegionNerestBase=%d\n", team->m_RegionNerestBase);
            fprintf(fi, "\n");
            fprintf(fi, "    m_RobotCnt=%d\n", team->m_RobotCnt);
            fprintf(fi, "    m_War=%d\n", int(team->IsWar()));
            fprintf(fi, "\n");
            fprintf(fi, "    m_Action.m_Type=%s\n", ats[team->m_Action.m_Type]);
            fprintf(fi, "    m_Action.m_Region=%d\n", team->m_Action.m_Region);
            fprintf(fi, "\n");

            for(int ig = 0; ig < MAX_LOGIC_GROUP; ++ig)
            {
                if(m_Side[is].m_LogicGroup[ig].RobotsCnt() <= 0) continue;
                if(m_Side[is].m_LogicGroup[ig].m_Team != it) continue;

                SMatrixLogicGroup *group = m_Side[is].m_LogicGroup + ig;

                fprintf(fi, "        --- Group(%d) ---\n", ig);

                fprintf(fi, "        m_RobotCnt=%d\n", group->RobotsCnt());
                fprintf(fi, "        m_Action.m_Type=%s\n", ats[group->m_Action.m_Type]);
                fprintf(fi, "        m_Action.m_Region=%d\n", group->m_Action.m_Region);
                fprintf(fi, "        m_War=%d\n", int(group->IsWar()));
                fprintf(fi, "\n");
                
                CMatrixMapStatic *ms = CMatrixMapStatic::GetFirstLogic();
                while(ms)
                {
                    if(ms->IsRobot() && ms->AsRobot()->m_Side == m_Side[is].m_Id && ms->AsRobot()->GetGroupLogic() == ig)
                    {
                        CMatrixRobotAI *robot = (CMatrixRobotAI*)ms;
                        CInfo *env = robot->GetEnv();

                        fprintf(fi, "            --- Robot(%p) ---\n", ms);

                        fprintf(fi, "            m_MapPos=%d,%d    Region=%d\n", robot->GetMapPosX(), robot->GetMapPosY(), robot->GetRegion());
                        fprintf(fi, "            m_Environment.m_EnemyCnt=%d\n", env->m_EnemyCnt);
                        if(env->m_Place < 0) fprintf(fi, "            m_Environment.m_Place=%d\n", env->m_Place);
                        else fprintf(fi, "            m_Environment.m_Place=%d    Region=%d\n", env->m_Place, g_MatrixMap->m_RoadNetwork.GetPlace(env->m_Place)->m_Region);
                        fprintf(fi, "            m_Environment.m_OrderNoBreak=%d\n", env->m_OrderNoBreak);
                        fprintf(fi, "            m_CurrentState=%s\n", rss[robot->m_CurrentState]);

                        for(int i = 0; i < robot->GetOrdersInPool(); ++i)
                        {
                            SOrder* order = robot->GetOrder(i);

                            fprintf(fi, "            Order.%d=%s %s\n", i, ros[order->GetOrderType()], rps[order->GetOrderPhase()]);
                        }

                        fprintf(fi, "\n");
                    }
                    ms = ms->GetNextLogic();
                }
            }
        }
    }

    fclose(fi);
}