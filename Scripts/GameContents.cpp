#include <winsock2.h>
#include <iostream>
#include "RingBuffer.h"
#include <list>
#include <unordered_map>
#include "Structs.h"
#include "SectorManager.h"
#include "PlayerManager.h"
#include "PacketBuffer.h"
#include "MemoryPool.h"

#include "IStub.h"
#include "GameStub.h"
#include "GameProxy.h"
#include "GameContents.h"
#include "NetworkUtils.h"
#include "LogManager.h"
#include "Profiler.h"

using namespace std;

//---------------------------------------------------------------------
// [생성자]
// - 패킷 풀 초기화
//---------------------------------------------------------------------
CGameContents::CGameContents():_packetPool(1)
{

}

//---------------------------------------------------------------------
// [MovePlayer]
// - 플레이어 이동 처리 및 섹터 변경 감지
//---------------------------------------------------------------------
void CGameContents::MovePlayer(Player* player)
{
    int oldX = player->x;
    int oldY = player->y;

    // 이동 전 섹터 정보 저장
    player->oldSector = player->curSector;

    // 이동 방향에 따른 좌표 변경
    switch (player->moveDir)
    {
    case dfPACKET_MOVE_DIR_LL:
        if (player->x - 3 > dfRANGE_MOVE_LEFT)
        {
            player->x -= 3;
        }
        break;

    case dfPACKET_MOVE_DIR_LU:
        if (player->x - 3 > dfRANGE_MOVE_LEFT && player->y - 2 > dfRANGE_MOVE_TOP)
        {
            player->x -= 3;
            player->y -= 2;
        }
        break;

    case dfPACKET_MOVE_DIR_UU:
        if (player->y - 2> dfRANGE_MOVE_TOP)
        {
            player->y -= 2;
        }
        break;

    case dfPACKET_MOVE_DIR_RU:
        if (player->x + 3 < dfRANGE_MOVE_RIGHT && player->y - 2> dfRANGE_MOVE_TOP)
        {
            player->x += 3;
            player->y -= 2;
        }
        break;

    case dfPACKET_MOVE_DIR_RR:
        if (player->x + 3 < dfRANGE_MOVE_RIGHT)
        {
            player->x += 3;
        }
        break;

    case dfPACKET_MOVE_DIR_RD:
        if (player->x + 3 < dfRANGE_MOVE_RIGHT && player->y + 2 < dfRANGE_MOVE_BOTTOM)
        {
            player->x += 3;
            player->y += 2;
        }
        break;

    case dfPACKET_MOVE_DIR_DD:
        if (player->y + 2 < dfRANGE_MOVE_BOTTOM)
        {
            player->y += 2;
        }
        break;

    case dfPACKET_MOVE_DIR_LD:
        if (player->x - 3 > dfRANGE_MOVE_LEFT && player->y + 2 < dfRANGE_MOVE_BOTTOM)
        {
            player->x -= 3;
            player->y += 2;
        }
        break;

    }

    // 섹터 변경 감지
    player->curSector.x = player->x / SECTOR_SIZE;
    player->curSector.y = player->y / SECTOR_SIZE;

    // 섹터가 변경되면 시야 갱신
    if (player->oldSector != player->curSector)
    {
        _sectorManager->Move(player, player->oldSector, player->curSector);
        UpdateSight(player);
    }
    
}

//---------------------------------------------------------------------
// [UpdateSight]
// - 플레이어의 시야 내/외부 변화 감지 및 캐릭터 생성/삭제 처리
//---------------------------------------------------------------------
void CGameContents::UpdateSight(Player* player)
{
    int oldSectorX = player->oldSector.x;
    int oldSectorY = player->oldSector.y;
    int newSectorX = player->curSector.x;
    int newSectorY = player->curSector.y;

    // 주변 시야 목록
    SectorAround oldAround = _sectorManager->GetSectorAround(oldSectorX, oldSectorY);
    SectorAround newAround = _sectorManager->GetSectorAround(newSectorX, newSectorY);

    vector<SectorPos> addSectors;
    vector<SectorPos> removeSectors;

    // 이전 시야에서 사라진 섹터 계산
    for (int i = 0; i < oldAround.cnt; ++i)
    {
        SectorPos oldPos = oldAround.around[i];
        bool found = false;

        for (int j = 0; j < newAround.cnt; j++)
        {
            if (IsSameSector(oldPos, newAround.around[j]))
            {
                found = true;
                break;
            }
        }

        if (!found)
            removeSectors.push_back(oldPos);
    }

    // 새롭게 추가된 섹터 계산
    for (int i = 0; i < newAround.cnt; ++i)
    {
        SectorPos newPos = newAround.around[i];
        bool found = false;

        for (int j = 0; j < oldAround.cnt; ++j)
        {
            if (IsSameSector(newPos, oldAround.around[j]))
            {
                found = true;
                break;
            }
        }

        if (!found)
            addSectors.push_back(newPos);
    }

    // 삭제 메시지 보내기
    for (const SectorPos& s : removeSectors)
    {
        const auto& list = _sectorManager->GetPlayerInSector(s.x, s.y);
        for (Player* target : list)
        {
            if (target == player || !target->isAlive)
                continue;

            // 나를 삭제해라
            CPacket* packet = _packetPool.Alloc();
            packet->Clear();
            _proxy->mpDeleteCharacter(target, packet, player->id);
            _packetPool.Free(packet);

            // 상대를 삭제해라
            CPacket* packet2 = _packetPool.Alloc();
            packet2->Clear();
            _proxy->mpDeleteCharacter(player, packet2, target->id);
            _packetPool.Free(packet2);
        }
    }

    // 생성 메시지 보내기
    for (const SectorPos& s : addSectors)
    {
        const auto& list = _sectorManager->GetPlayerInSector(s.x, s.y);
        for (Player* target : list)
        {
            if (target == player || !target->isAlive)
                continue;

            // 나를 생성해라
            CPacket* packet = _packetPool.Alloc();
            packet->Clear();
            _proxy->mpCreateOtherCharacter(target, packet, player->id, player->direction, player->x, player->y, player->hp);
            _packetPool.Free(packet);

            // 내가 이동중이라면 이동 패킷 추가 전송
            if (player->state == dfMove)
            {
                CPacket* movePacket = _packetPool.Alloc();
                movePacket->Clear();
                _proxy->mpMoveStart(target, movePacket, player->id, player->moveDir, player->x, player->y);
                _packetPool.Free(movePacket);
            }

            // 상대를 생성해라
            CPacket* packet2 = _packetPool.Alloc();
            packet2->Clear();
            _proxy->mpCreateOtherCharacter(player, packet2, target->id, target->direction, target->x, target->y, target->hp);
            _packetPool.Free(packet2);

            if (target->state == dfMove)
            {
                CPacket* movePacket = _packetPool.Alloc();
                movePacket->Clear();
                _proxy->mpMoveStart(player, movePacket, target->id, target->moveDir, target->x, target->y);
                _packetPool.Free(movePacket);
            }
        }
    }
}

//---------------------------------------------------------------------
// [IsSameSector]
// - 두 섹터의 좌표가 같은지 비교
//---------------------------------------------------------------------
bool CGameContents::IsSameSector(const SectorPos& a, const SectorPos& b)
{
    return a.x == b.x && a.y == b.y;
}

//---------------------------------------------------------------------
// [MsgProcMoveStart]
// - 이동 시작 메시지 처리
// - 좌표 동기화 확인 후 이동 방향, 상태 갱신
//---------------------------------------------------------------------
bool CGameContents::MsgProcMoveStart(Player* player, CPacket* payroad)
{
    // 패킷(페이로드)에서 데이터 빼기
    BYTE direction;
    WORD x;
    WORD y;

    *payroad >> direction >> x >> y;

    // 좌표 오차 범위 검증
    if (abs(player->x - x) > dfERROR_RANGE ||
        abs(player->y - y) > dfERROR_RANGE)
    {
        // Sync 메시지 전송 (보정)
        CPacket* sendPacket = _packetPool.Alloc();
        sendPacket->Clear();
        //_proxy->mpSyncAround(player, sendPacket, player->id, player->x, player->y, true);
        _proxy->mpSync(player, sendPacket, player->id, player->x, player->y);
        _packetPool.Free(sendPacket);

        x = player->x;
        y = player->y;
    }

    // 이동 상태 갱신
    player->state = dfMove;
    // 플레이어 방향 변경
    switch (direction)
    {
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RD:
        player->direction = dfPACKET_MOVE_DIR_RR;
        break;
    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LD:
        player->direction = dfPACKET_MOVE_DIR_LL;
        break;
    }
    player->moveDir = direction;
    player->x = x;
    player->y = y;

    // 주변 유저에게 이동 시작 알림
    CPacket* sendPacket = _packetPool.Alloc();
    sendPacket->Clear();
    _proxy->mpMoveStartAround(player, sendPacket, player->id, direction, x, y);
    _packetPool.Free(sendPacket);

    // 세션 시간 갱신
    player->session->lastRecvTime = timeGetTime();

    return TRUE;
}

//---------------------------------------------------------------------
// [MsgProcMoveStop]
// - 이동 정지 메시지 처리
//---------------------------------------------------------------------
bool CGameContents::MsgProcMoveStop(Player* player, CPacket* payroad)
{
    BYTE direction;
    WORD x;
    WORD y;

    *payroad >> direction >> x >> y;

    // 좌표 오차 검증
    if (abs(player->x - x) > dfERROR_RANGE ||
        abs(player->y - y) > dfERROR_RANGE)
    {
        // Sync 메시지 전송 (보정)
        CPacket* sendPacket = _packetPool.Alloc();
        sendPacket->Clear();
        //_proxy->mpSyncAround(player, sendPacket, player->id, player->x, player->y, true);
        _proxy->mpSync(player, sendPacket, player->id, player->x, player->y);
        _packetPool.Free(sendPacket);

        x = player->x;
        y = player->y;
    }

    // 플레이어 방향 변경
    switch (direction)
    {
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RD:
        player->direction = dfPACKET_MOVE_DIR_RR;
        break;
    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LD:
        player->direction = dfPACKET_MOVE_DIR_LL;
        break;
    }
    // 플레이어 상태 변경
    player->x = x;
    player->y = y;
    player->state = dfStop;

    // 주변에 정지 패킷 전송
    CPacket* sendPacket = _packetPool.Alloc();
    sendPacket->Clear();
    _proxy->mpMoveStopAround(player, sendPacket, player->id, direction, x, y);
    _packetPool.Free(sendPacket);

    // 세션 시간 갱신
    player->session->lastRecvTime = timeGetTime();

    return TRUE;
}

//---------------------------------------------------------------------
// [MsgProcAttack1/2/3]
// - 공격 메시지 처리 (범위/타입별 데미지 및 판정)
//---------------------------------------------------------------------
bool CGameContents::MsgProcAttack1(Player* player, CPacket* payroad)
{
    BYTE direction;
    WORD x;
    WORD y;

    *payroad >> direction >> x >> y;

    // 방향 전환
    player->direction = direction;

    // 주변 플레이어에게 공격 알림
    CPacket* sendPacket = _packetPool.Alloc();
    sendPacket->Clear();
    _proxy->mpAttack1Around(player, sendPacket, player->id, direction, x, y, true);
    _packetPool.Free(sendPacket);

    // 섹터 기반 충돌 판정
    SectorAround around = _sectorManager->GetSectorAround(player->curSector.x, player->curSector.y);
    for (int i = 0; i < around.cnt; i++)
    {
        int sx = around.around[i].x;
        int sy = around.around[i].y;

        for (Player* target : _sectorManager->_sectors[sy][sx])
        {
            if (target == player || !target->isAlive)
                continue;

            if (abs(target->x - x) < dfATTACK1_RANGE_X &&
                abs(target->y - y) < dfATTACK1_RANGE_Y)
            {
                CPacket* sendPacket1 = _packetPool.Alloc();
                sendPacket1->Clear();

                if (direction == dfPACKET_MOVE_DIR_LL && (target->x < x))
                {
                    target->hp -= dfDAMAGE;

                    _proxy->mpDamageAround(target, sendPacket1, player->id, target->id, target->hp, true);
                    _packetPool.Free(sendPacket1);

                    if (target->hp <= 0)
                    {
                        _netUtils->DisconnectPlayer(target);
                    }

                    continue;
                }
                else if (direction == dfPACKET_MOVE_DIR_RR && (target->x > x))
                {
                    target->hp -= dfDAMAGE;

                    _proxy->mpDamageAround(target, sendPacket1, player->id, target->id, target->hp, true);
                    _packetPool.Free(sendPacket1);

                    if (target->hp <= 0)
                    {
                        _netUtils->DisconnectPlayer(target);
                    }

                    continue;
                }

                _packetPool.Free(sendPacket1);

            }

        }
    }

    // 세션 시간 갱신
    player->session->lastRecvTime = timeGetTime();

    return TRUE;
}

bool CGameContents::MsgProcAttack2(Player* player, CPacket* payroad)
{
    BYTE direction;
    WORD x;
    WORD y;

    *payroad >> direction >> x >> y;

    // 방향 전환
    player->direction = direction;

    // 공격 메시지 생성
    CPacket* sendPacket = _packetPool.Alloc();
    sendPacket->Clear();
    _proxy->mpAttack2Around(player, sendPacket, player->id, direction, x, y);
    _packetPool.Free(sendPacket);

    // 충돌 판정
    SectorAround around = _sectorManager->GetSectorAround(player->curSector.x, player->curSector.y);

    for (int i = 0; i < around.cnt; i++)
    {
        int sx = around.around[i].x;
        int sy = around.around[i].y;

        for (Player* target : _sectorManager->_sectors[sy][sx])
        {
            if (target == player || !target->isAlive)
                continue;

            if (abs(target->x - x) < dfATTACK2_RANGE_X &&
                abs(target->y - y) < dfATTACK2_RANGE_Y)
            {
                CPacket* sendPacket1 = _packetPool.Alloc();
                sendPacket1->Clear();

                if (direction == dfPACKET_MOVE_DIR_LL && (target->x < x))
                {
                    target->hp -= dfDAMAGE;

                    _proxy->mpDamageAround(target, sendPacket1, player->id, target->id, target->hp, true);
                    _packetPool.Free(sendPacket1);

                    if (target->hp <= 0)
                    {
                        _netUtils->DisconnectPlayer(target);
                    }

                    continue;

                }
                else if (direction == dfPACKET_MOVE_DIR_RR && (target->x > x))
                {
                    target->hp -= dfDAMAGE;

                    _proxy->mpDamageAround(target, sendPacket1, player->id, target->id, target->hp, true);
                    _packetPool.Free(sendPacket1);

                    if (target->hp <= 0)
                    {
                        _netUtils->DisconnectPlayer(target);
                    }

                    continue;
                }

                _packetPool.Free(sendPacket1);

            }
        }
    }

    // 세션 시간 갱신
    player->session->lastRecvTime = timeGetTime();

    return TRUE;

}

bool CGameContents::MsgProcAttack3(Player* player, CPacket* payroad)
{
    BYTE direction;
    WORD x;
    WORD y;

    *payroad >> direction >> x >> y;

    // 방향 전환
    player->direction = direction;

    // 공격 메시지 생성
    CPacket* sendPacket = _packetPool.Alloc();
    sendPacket->Clear();
    _proxy->mpAttack3Around(player, sendPacket, player->id, direction, x, y);
    _packetPool.Free(sendPacket);

    // 충돌 판정
    SectorAround around = _sectorManager->GetSectorAround(player->curSector.x, player->curSector.y);

    for (int i = 0; i < around.cnt; i++)
    {
        int sx = around.around[i].x;
        int sy = around.around[i].y;

        for (Player* target : _sectorManager->_sectors[sy][sx])
        {
            if (target == player || !target->isAlive)
                continue;

            if (abs(target->x - x) < dfATTACK3_RANGE_X &&
                abs(target->y - y) < dfATTACK3_RANGE_Y)
            {
                CPacket* sendPacket1 = _packetPool.Alloc();
                sendPacket1->Clear();

                if (direction == dfPACKET_MOVE_DIR_LL && (target->x < x))
                {
                    target->hp -= dfDAMAGE;

                    _proxy->mpDamageAround(target, sendPacket1, player->id, target->id, target->hp, true);
                    _packetPool.Free(sendPacket1);

                    if (target->hp <= 0)
                    {
                        _netUtils->DisconnectPlayer(target);
                    }

                    continue;

                }
                else if (direction == dfPACKET_MOVE_DIR_RR && (target->x > x))
                {
                    target->hp -= dfDAMAGE;

                    _proxy->mpDamageAround(target, sendPacket1, player->id, target->id, target->hp, true);
                    _packetPool.Free(sendPacket1);

                    if (target->hp <= 0)
                    {
                        _netUtils->DisconnectPlayer(target);
                    }

                    continue;
                }

                _packetPool.Free(sendPacket1);

            }
        }
    }


    // 세션 시간 갱신
    player->session->lastRecvTime = timeGetTime();

    return TRUE;

}

//---------------------------------------------------------------------
// [MsgEcho]
// - 클라이언트와의 Echo 응답 테스트 (핑 체크)
//---------------------------------------------------------------------
bool CGameContents::MsgEcho(Player* player, CPacket* payroad)
{
    DWORD time;

    *payroad >> time;

    CPacket* sendPacket = _packetPool.Alloc();
    sendPacket->Clear();
    _proxy->mpEcho(player, sendPacket, time);
    _packetPool.Free(sendPacket);

    // 세션 시간 갱신
    player->session->lastRecvTime = timeGetTime();
    
    return TRUE;
}

//---------------------------------------------------------------------
// [Setter Functions]
// - 모듈 간 의존성 주입
//---------------------------------------------------------------------
void CGameContents::SetNetworkUtils(CNetworkUtils* netUtils)
{
    _netUtils = netUtils;
}

void CGameContents::SetProxy(CGameProxy* proxy)
{
    _proxy = proxy;
}

void CGameContents::SetSectorManager(CSectorManager* sectorManager)
{
    _sectorManager = sectorManager;
}

//---------------------------------------------------------------------
// [UpdateGame]
// - 프레임 단위 게임 로직 업데이트
//   • 타임아웃 플레이어 처리
//   • 이동 상태 플레이어 위치 갱신
//---------------------------------------------------------------------
bool CGameContents::UpdateGame()
{
    DWORD currentTick = timeGetTime();

    for (auto& [id, player] : _mapPlayer)
    {
        if (!(player->isAlive) || !(player->session->isAlive))
            continue;

        // 일정 시간 패킷 수신 없을 경우 연결 종료
        if (currentTick - player->session->lastRecvTime >= dfNETWORK_PACKET_RECV_TIMEOUT)
        {
            _netUtils->DisconnectPlayer(player);
            continue;
        }

        // 이동 상태일 경우 위치 갱신
        if (player->state == dfMove)
        {
            MovePlayer(player);
        }

    }

    return true;
}

//---------------------------------------------------------------------
// [플레이어 관리 함수]
//---------------------------------------------------------------------
void CGameContents::AddPlayer(Player* player)
{
    _mapPlayer[player->id] = player;
}

Player* CGameContents::GetPlayer(DWORD sessionId)
{
    unordered_map<DWORD, Player*>::iterator iter = _mapPlayer.find(sessionId);

    if (iter != _mapPlayer.end())
        return iter->second;
    else
        return nullptr;
}

void CGameContents::RemovePlayer(DWORD sessionId)
{
    unordered_map<DWORD, Player*>::iterator iter = _mapPlayer.find(sessionId);
    
    if (iter != _mapPlayer.end())
    {
        delete iter->second;
        _mapPlayer.erase(iter);
    }

}

//---------------------------------------------------------------------
// [메모리풀 상태 조회]
//---------------------------------------------------------------------
int CGameContents::GetPacketPoolCapacity()
{
    return _packetPool.GetCapacityCount();
}

int CGameContents::GetPacketPoolUseCount()
{
    return _packetPool.GetUseCount();
}
