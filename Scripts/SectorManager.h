//=====================================================================
//  [클래스 개요]
//  CSectorManager
//   - 맵을 2차원 섹터(격자) 단위로 나누어 플레이어의 시야 및 충돌 범위를 관리
//=====================================================================
//  [주요 기능]
//   1) Enter()  : 플레이어가 섹터에 진입할 때 등록
//   2) Leave()  : 플레이어가 섹터에서 이탈할 때 제거
//   3) Move()   : 섹터 간 이동 처리 (기존 섹터 제거 → 새 섹터 등록)
//   4) GetSectorAround() : 3×3 범위(자기 자신 + 주변 8섹터) 반환
//   5) GetPlayerInSector(): 특정 섹터의 플레이어 목록 조회
//=====================================================================

#pragma once
#include <list>
#include <vector>
#include <algorithm>

#define SECTOR_MAX_X 128
#define SECTOR_MAX_Y 128
#define SECTOR_SIZE 200  // 한 섹터의 크기 (픽셀 단위)

using namespace std;

struct Player;
struct SectorAround
{
	int cnt;				// 주변 섹터 개수
	SectorPos around[9];	// 주변 섹터 좌표 (최대 9개)
};

class CSectorManager
{
public:
	//-----------------------------------------------------------------
	// [멤버 변수]
	//-----------------------------------------------------------------
	list<Player*> _sectors[SECTOR_MAX_Y][SECTOR_MAX_X]; // 섹터별 플레이어 리스트

	//-----------------------------------------------------------------
	// [주요 기능]
	//-----------------------------------------------------------------
	void Enter(Player* player, int x, int y);					// 플레이어 섹터 진입
	void Leave(Player* player, const SectorPos& sector);		// 플레이어 섹터 이탈
	void Move(Player* player, const SectorPos& oldSector, const SectorPos& newSector); // 섹터 간 이동

	SectorAround GetSectorAround(int sx, int sy);				// 주변 3X3 섹터 반환
	const list<Player*>& GetPlayerInSector(int x, int y) const; // 특정 섹터 내 플레이어 목록 반환


private:
	//-----------------------------------------------------------------
	// [좌표 → 섹터 인덱스 변환 함수]
	//-----------------------------------------------------------------
	inline int ToSectorX(int x) { return x / SECTOR_SIZE; };
	inline int ToSectorY(int y) { return y / SECTOR_SIZE; };
};