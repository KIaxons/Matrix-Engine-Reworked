// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

#include "MatrixMap.hpp"

extern CMatrixRobotAI* g_TestRobot;
extern bool g_TestLocal;

#define MAX_UNIT_ON_PATH  64

struct SMatrixPathUnit {
    float sx = 0.0f, sy = 0.0f; // Начало (мировые координаты но кратные GLOBAL_SCALE_MOVE)
    float ex = 0.0f, ey = 0.0f; // Конец (мировые координаты но кратные GLOBAL_SCALE_MOVE)
    float vx = 0.0f, vy = 0.0f; // Направление (нормализированно)
    float length = 0.0f; // Длина
};

struct SMatrixPath;

struct SMatrixPathObj {
    SMatrixPathObj* m_Prev = nullptr;
    SMatrixPathObj* m_Next = nullptr;

    SMatrixPath* m_Path = nullptr;

    float cx = 0.0f, cy = 0.0f;    // Где объект сейчас находится
    float tx = 0.0f, ty = 0.0f;    // Где объект будит находится
    float radius = 0.0f;   // Радиус объекта идущего по пути
    int nsh = 0;
    int size = 0;

    bool calc_pos = false;
};

struct SMatrixPath  {
    SMatrixPath* m_Prev = nullptr;
    SMatrixPath* m_Next = nullptr;
    int m_Cnt = 0;
    SMatrixPathUnit m_Module[MAX_UNIT_ON_PATH];

    SMatrixPathObj* m_Owner = nullptr;

    float m_StartX = 0.0f;
    float m_StartY = 0.0f; // Boundbox для пути
    float m_EndX = 0.0f;
    float m_EndY = 0.0f;
};

class CMatrixMapLogic : public CMatrixMap {
public:
    //SMatrixMapZone* m_Zone[5] = { nullptr };
    //int m_ZoneCnt[5] = { 0 };

    SMatrixPath* m_PathFirst = nullptr;
    SMatrixPath* m_PathLast = nullptr;
    SMatrixPathObj* m_ObjFirst = nullptr;
    SMatrixPathObj* m_ObjLast = nullptr;

    int m_MPFCnt = 0;
    int m_MPF2Cnt = 0;
    CPoint* m_MPF = nullptr;
    CPoint* m_MPF2 = nullptr;

    int* m_ZoneIndex = nullptr;
    int* m_ZoneIndexAccess = nullptr;
    dword* m_ZoneDataZero = nullptr;

    CPoint* m_MapPoint = nullptr;

	//int m_Tact = 0; //Game tact
	int m_TactNext = 0;

	int m_Rnd = 0;

    int m_GatherInfoLast = 0;

    CMatrixMapLogic()
    {
        m_Rnd = rand();
        Rnd(0, 1);
    }
	~CMatrixMapLogic();

	void Clear();

	int   Rnd();
    float RndFloat(); // 0-1
    int   Rnd(int zmin, int zmax);
    float RndFloat(float zmin, float zmax);

    void GatherInfo(int type);

    void PrepareBuf();

    //void ZoneClear();
    //void ZoneCalc(int nsh, byte mm);
    //void ZoneCalc();
    int ZoneFindNear(int nsh, int mx, int my);
    void PlaceGet(int nsh, float wx, float wy, int* mx, int* my);
    bool IsAbsenceWall(int nsh, int size, int mx, int my);
    byte GetCellMoveType(int nsh, int mx, int my) // ff-free 0-box 1-sphere
    {
        SMatrixMapMove* smm = MoveGetTest(mx, my);
        if(!smm) return 0xff;
        return smm->GetType(nsh);
    }

    //bool PlaceFindNear(int nsh, int size, int& mx, int& my);
    bool PlaceFindNear(int nsh, int size, int& mx, int& my, int other_cnt, int* other_size, CPoint* other_des);
    //bool PlaceFindNear(int nsh, int size, int& mx, int& my, const D3DXVECTOR2& vdir, int other_cnt, int* other_size, CPoint* other_des);
    bool PlaceFindNear(int nsh, int size, int& mx, int& my, CMatrixMapStatic* skip);
    bool PlaceFindNearReturn(int nsh, int size, int& mx, int& my, CMatrixRobotAI* robot);
    //bool PlaceFindNear(int nsh, int size, int& mx, int& my, const D3DXVECTOR2& vdir, CMatrixMapStatic* skip);
    bool PlaceIsEmpty(int nsh, int size, int mx, int my, CMatrixMapStatic* skip);

    //int ZoneMoveFindNear(int nsh, int mx, int my, CPoint* path);
    //int ZoneMoveFind(int nsh, int size, int mx, int my, int zonesou1, int zonesou2, int zonesou3, int deszone, int dx, int dy, CPoint* path);
    //int ZoneMoveIn(int nsh, int size, int sx, int sy, int dx, int dy, CPoint* path);
    //int ZoneFindPath(int nsh, int zstart, int zend, int* path);

    void SetWeightFromTo(int size, int x1, int y1, int x2, int y2);
    int FindLocalPath(
        int nsh, int size,
        int mx, int my,           // Начальная точка
        int* zonepath, int zonepathcnt, // Список зон через которые нужной найти путь
        int dx, int dy,           // Точка назначения
        CPoint* path,             // Рассчитанный путь
        int other_cnt,            // Кол-во путей от других роботов
        int* other_size,          // Список размеров в других путях
        CPoint** other_path_list, // Список указателей на другие пути
        int* other_path_cnt,      // Список кол-во элементов в других путях
        CPoint* other_des,        // Список конечных точек в других путях
        bool test
    );

    void SetZoneAccess(int* list, int cnt, bool value);
    int FindPathInZone(int nsh, int zstart, int zend, const CMatrixRoadRoute* route, int routeno, int* path, bool test);
    bool CanMoveFromTo(int nsh, int size, int x1, int y1, int x2, int y2, CPoint* path);
    bool CanOptimize(int nsh, int size, int x1, int y1, int x2, int y2);
    int OptimizeMovePath(int nsh, int size, int cnt, CPoint* path);
    int OptimizeMovePathSimple(int nsh, int size, int cnt, CPoint* path);
    int RandomizeMovePath(int nsh, int size, int cnt, CPoint* path);
    int FindNearPlace(byte mm, const CPoint& mappos);
    int FindPlace(const CPoint& mappos);

    int PlaceList(byte mm, const CPoint& from, const CPoint& to, int radius, bool farpath, int* list, int* listcnt, int* outdist = nullptr); // Return 0-not found 1-can move to 2-barrier
    int PlaceListGrow(byte mm, int* list, int* listcnt, int growcnt);

    SMatrixPathObj* ObjAdd();
    void ObjDelete(SMatrixPathObj* obj);
    void ObjClear();

    SMatrixPath* PathAlloc();
    void PathFree(SMatrixPath* path);
    void PathClear();
    void PathAdd(SMatrixPath* path) { LIST_ADD(path, m_PathFirst, m_PathLast, m_Prev, m_Next); }
    void PathInsert(SMatrixPath* ito, SMatrixPath* path) { LIST_INSERT(ito, path, m_PathFirst, m_PathLast, m_Prev, m_Next); }
    void PathDelete(SMatrixPath* path, bool free = true) { LIST_DEL(path, m_PathFirst, m_PathLast, m_Prev, m_Next); if(free) PathFree(path); }
    void PathCalcPosAfter(SMatrixPath* path = nullptr);
    bool PathCheckInFindInsert(SMatrixPath* path);
    //void PathCheckIn(SMatrixPath* path);
    bool PathCheckInFindPos(SMatrixPath* path, SMatrixPathObj* obj);
    bool PathIntersectAfter(SMatrixPath* path);
    bool PathIntersect(SMatrixPath* path, float cx, float cy, float radius);
    //bool PathIntersect(SMatrixPath* path1, SMatrixPath* path2);

    inline int GetRegion(const CPoint& tp);
    inline int GetRegion(int x, int y);

    void CalcCannonPlace();

    void Tact(int step);

    bool IsLogicVisible(CMatrixMapStatic* ofrom, CMatrixMapStatic* oto, float second_z = 0.0f);

    void DumpLogic();
};

inline int CMatrixMapGroup::ObjectsCnt() const
{
    return m_ObjectsContained;
}
inline CMatrixMapStatic* CMatrixMapGroup::ReturnObject(int i) const
{
    return m_Objects[i];
}

inline int CMatrixMapLogic::GetRegion(const CPoint& tp)
{
    while(true)
    {
        SMatrixMapMove* mm = g_MatrixMap->MoveGetTest(tp.x, tp.y);
        if(mm == nullptr) break;
        if(mm->m_Zone < 0) break;
        if(g_MatrixMap->m_RoadNetwork.m_Zone[mm->m_Zone].m_Region < 0) break;
        return g_MatrixMap->m_RoadNetwork.m_Zone[mm->m_Zone].m_Region;
    }
    return g_MatrixMap->m_RoadNetwork.FindNearestRegion(tp);
}

inline int CMatrixMapLogic::GetRegion(int x, int y)
{
    while(true)
    {
        SMatrixMapMove* mm = g_MatrixMap->MoveGetTest(x, y);
        if(mm == nullptr) break;
        if(mm->m_Zone < 0) break;
        if(g_MatrixMap->m_RoadNetwork.m_Zone[mm->m_Zone].m_Region < 0) break;
        return g_MatrixMap->m_RoadNetwork.m_Zone[mm->m_Zone].m_Region;
    }

    return g_MatrixMap->m_RoadNetwork.FindNearestRegion(CPoint(x, y));
}