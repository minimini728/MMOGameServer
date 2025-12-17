#include <winsock2.h>
#include <iostream>
#include "RingBuffer.h"
#include <list>
#include <unordered_map>
#include "Structs.h"
#include "MemoryPool.h"

#include "SectorManager.h"

//---------------------------------------------------------------------
// [Enter]
// - 플레이어를 해당 위치의 섹터에 등록
//---------------------------------------------------------------------
void CSectorManager::Enter(Player* player, int x, int y)
{
	int sx = ToSectorX(x);
	int sy = ToSectorY(y);

	// 유효 범위 검사
	if (sx < 0 || sy < 0 || sx >= SECTOR_MAX_X || sy >= SECTOR_MAX_Y)
		return;

	_sectors[sy][sx].push_back(player);
}

//---------------------------------------------------------------------
// [Leave]
// - 플레이어를 현재 섹터에서 제거
//---------------------------------------------------------------------
void CSectorManager::Leave(Player* player, const SectorPos& sector)
{
	if (sector.x < 0 || sector.y < 0 || sector.x >= SECTOR_MAX_X || sector.y >= SECTOR_MAX_Y)
		return;

	_sectors[sector.y][sector.x].remove(player);
}

//---------------------------------------------------------------------
// [Move]
// - 플레이어가 섹터를 이동했을 때, 이전 섹터에서 제거 후 새 섹터에 등록
//---------------------------------------------------------------------
void CSectorManager::Move(Player* player, const SectorPos& oldSector, const SectorPos& newSector)
{
	// 처음 접속 시 oldSector = (-1, -1) 일 수 있음
	if (oldSector.x != -1 && oldSector.y != -1)
	{
		_sectors[oldSector.y][oldSector.x].remove(player);
	}

	_sectors[newSector.y][newSector.x].push_back(player);
}

//---------------------------------------------------------------------
// [GetSectorAround]
// - 현재 섹터(sx, sy) 기준 3×3 범위의 주변 섹터 반환
// - 맵 경계를 벗어나는 섹터는 제외
//---------------------------------------------------------------------
SectorAround CSectorManager::GetSectorAround(int sx, int sy)
{
	SectorAround result;
	result.cnt = 0;

	const int dx[9] = { -1,  0, 1, -1, 0, 1, -1, 0, 1 };
	const int dy[9] = { -1, -1, -1,  0, 0, 0,  1, 1, 1 };

	for (int i = 0; i < 9; i++)
	{
		int nx = sx + dx[i];
		int ny = sy + dy[i];

		if (nx < 0 || ny < 0 || nx >= SECTOR_MAX_X || ny >= SECTOR_MAX_Y)
			continue;

		result.around[result.cnt++] = { nx, ny };
	}

	return result;
}

//---------------------------------------------------------------------
// [GetPlayerInSector]
// - 특정 섹터 내의 플레이어 리스트를 반환
//---------------------------------------------------------------------
const list<Player*>& CSectorManager::GetPlayerInSector(int x, int y) const
{
	return _sectors[y][x];
}

