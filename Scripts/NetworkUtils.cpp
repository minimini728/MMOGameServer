#include <winsock2.h>
#include <vector>
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
#include "GameProxy.h"
#include "GameStub.h"
#include "GameContents.h"
#include "NetworkUtils.h"
#include "LogManager.h"
#include "Profiler.h"

using namespace std;

//---------------------------------------------------------------------
// [생성자]
// - 세션/패킷 메모리풀 초기화 및 소켓 벡터 예약
//---------------------------------------------------------------------
CNetworkUtils::CNetworkUtils(): _sessionPool(10500), _packetPool(1)
{
    _allSockets.reserve(10500);
}

//---------------------------------------------------------------------
// [RemovePlayer]
// - 접속이 종료된 플레이어 및 세션을 정리
//---------------------------------------------------------------------
void CNetworkUtils::RemovePlayer()
{
    CGameContents* contents = dynamic_cast<CGameContents*>(_stub);

    for (SOCKET sock : _removeSockets)
    {
        auto it = _mapSession.find(sock);
        if (it == _mapSession.end())
            continue;

        stSession* session = it->second;

        // Player 제거
        Player* player = contents->GetPlayer(session->sessionId);
        if (player != nullptr)
        {
            _sectorManager->Leave(player, player->curSector);
            contents->RemovePlayer(session->sessionId);
        }

        // select 순회 벡터에서 지우기
        auto vectorIt = find(_allSockets.begin(), _allSockets.end(), sock);
        if (vectorIt != _allSockets.end())
        {
            _allSockets.erase(vectorIt);
        }

        // session map에서 지우기
        closesocket(session->socket);
        _sessionPool.Free(session);
        _mapSession.erase(it);
    }

    _removeSockets.clear();
}

//---------------------------------------------------------------------
// [DisconnectPlayer]
// - 클라이언트 연결 종료 및 삭제 알림 패킷 송신
//---------------------------------------------------------------------
void CNetworkUtils::DisconnectPlayer(Player* player)
{
    if (player->isAlive && player->session->isAlive)
    {
        // 캐릭터 삭제 패킷 송신
        CPacket sendPacket;
        _proxy->mpDeleteCharacterAround(player, &sendPacket, player->id);
                
        // 삭제 플래그 처리
        player->isAlive = false;
        player->session->isAlive = false;

        _removeSockets.push_back(player->session->socket);

    }
    
}

//---------------------------------------------------------------------
// [SendUnicast]
// - 특정 플레이어에게 단일 송신
//---------------------------------------------------------------------
void CNetworkUtils::SendUnicast(Player* player, char* msg, int size)
{
    int retenq = 0;

    if (!player || !player->session || !player->isAlive || !player->session->isAlive)
        return;

    retenq = player->session->sendQ.Enqueue(msg, size);
    if (retenq != size)
    {
        _LOG(0, L"- Send Error! id: %d enqueue: %d retEnqueue %d\n", player->session->sessionId, size, retenq);
        DebugBreak();
    }

}

//---------------------------------------------------------------------
// [SendAround]
// - 주변 섹터 내 플레이어들에게 송신
//---------------------------------------------------------------------
void CNetworkUtils::SendAround(Player* player, char* msg, int size, bool sendMe)
{
    int retenq = 0;

    SectorAround around = _sectorManager->GetSectorAround(player->curSector.x, player->curSector.y);

    for (int i = 0; i < around.cnt; i++)
    {
        int sx = around.around[i].x;
        int sy = around.around[i].y;

        for (Player* target : _sectorManager->_sectors[sy][sx])
        {
            if (target == player && !sendMe) 
                continue;

            if (!target->isAlive || !target->session->isAlive)
                continue;

            // enqueue
            retenq = target->session->sendQ.Enqueue(msg, size);
            if (retenq != size)
            {
                _LOG(0, L"- Send Error! id: %d enqueue: %d retEnqueue %d\n", target->session->sessionId, size, retenq);
                DebugBreak();
            }

        }
    }
}

//---------------------------------------------------------------------
// [Attach]
// - Stub, Proxy, SectorManager 등록
//---------------------------------------------------------------------
void CNetworkUtils::AttachStub(IStub* stub)
{
    _stub = stub;
}
void CNetworkUtils::AttachProxy(CGameProxy* proxy)
{
    _proxy = proxy;
}
void CNetworkUtils::AttachSectorManager(CSectorManager* sectorManager)
{
    _sectorManager = sectorManager;
}

//---------------------------------------------------------------------
// [UpdateNetwork]
// - select() 기반으로 모든 세션의 송수신 이벤트 처리
//--------------------------------------------------------------------
bool CNetworkUtils::UpdateNetwork(SOCKET listen_sock)
{
    int GLE = 0;
    int retselect = 0;
    int retsend = 0;
    FD_SET readSet, writeSet;
    FD_ZERO(&readSet);

    // listen 소켓 감시
    FD_SET(listen_sock, &readSet);

    timeval Time;
    Time.tv_sec = 0;
    Time.tv_usec = 0;
    retselect = select(0, &readSet, NULL, NULL, &Time);

    if (retselect == SOCKET_ERROR) // 실패
    {
        GLE = WSAGetLastError();
        _LOG(0, L"[NetworkUtils] listen_sock select() error: %d", GLE);
        return FALSE;
    }
    else if (retselect > 0) // 성공
    {
        if (FD_ISSET(listen_sock, &readSet))
        {
            // accept()
            ProfileBegin("Accept");
            for (int i = 0; i < 20; i++)
            {
                if (!AcceptProc(listen_sock))
                    break;
            }
            ProfileEnd("Accept");
        }
    }

    // 기존 클라이언트 처리
    size_t vectorSize = _allSockets.size();
    for (int i = 0; i < vectorSize; i += 64)
    {
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        int end = min(i + 64, vectorSize);
        for (int j = i; j < end; j++)
        {
            SOCKET sock = _allSockets[j];
            FD_SET(sock, &readSet);

            if (_mapSession[sock]->sendQ.GetUseSize() > 0)
                FD_SET(sock, &writeSet);
        }

        Time = { 0, 0 };
        retselect = select(0, &readSet, &writeSet, NULL, &Time);
        if (retselect == SOCKET_ERROR) // 실패
        {
            GLE = WSAGetLastError();
            _LOG(0, L"[NetworkUtils] select() error: %d", GLE);
            DebugBreak();
            return FALSE;
        }
        else if (retselect > 0) // 성공
        {
            for (int j = i; j < end; ++j)
            {
                SOCKET sock = _allSockets[j];
                stSession* session = _mapSession[sock];

                // recv
                if (FD_ISSET(sock, &readSet))
                {
                    RecvProc(session);
                }

                // send
                if (FD_ISSET(sock, &writeSet))
                {
                    retsend = send(sock, session->sendQ.GetFrontBufferPtr(),
                        session->sendQ.DirectDequeueSize(), 0);

                    if (retsend == SOCKET_ERROR)
                    {
                        GLE = WSAGetLastError();
                        if (GLE == WSAECONNRESET || GLE == WSAECONNABORTED)
                        {
                            CGameContents* contents = dynamic_cast<CGameContents*>(_stub);
                            Player* player = contents->GetPlayer(session->sessionId);
                            if (player != nullptr)
                            {
                                DisconnectPlayer(player);
                                continue;
                            }
                        }
                        else if(GLE != WSAEWOULDBLOCK)
                        {
                            _LOG(0, L"[NetworkUtils] send() error: %d", GLE);
                            DebugBreak();
                            return FALSE;
                        }
                    }

                    session->sendQ.MoveFront(retsend);

                }
            }

        }
    }

    return TRUE;
}

//---------------------------------------------------------------------
// [AcceptProc]
// - 새 클라이언트 접속 처리 및 초기 캐릭터 생성
//---------------------------------------------------------------------
bool CNetworkUtils::AcceptProc(SOCKET listen_sock)
{
    int GLE;
    SOCKET clientSocket;
    SOCKADDR_IN clientAddr;
    int addrLen;
    static DWORD clientId = 1;

    // accept()
    addrLen = sizeof(clientAddr);
    clientSocket = accept(listen_sock, (SOCKADDR*)&clientAddr, &addrLen);
    if (clientSocket == INVALID_SOCKET)
    {
        GLE = WSAGetLastError();
        if (GLE == WSAEWOULDBLOCK)
            return false;
        else
        {
            _LOG(0, L"[NetworkUtils::AcceptProc] accept() error: %d", GLE);
            DebugBreak();
            return false;
        }
    }

    // 네이글 알고리즘 비활성화
    bool nagleFlag = TRUE;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&nagleFlag, sizeof(nagleFlag));

    // 세션 생성 및 추가
    stSession* session = _sessionPool.Alloc();
    session->socket = clientSocket;
    session->sessionId = clientId;
    session->lastRecvTime = timeGetTime();
    session->isAlive = true;
    session->recvQ.ClearBuffer();
    session->sendQ.ClearBuffer();

    _mapSession[clientSocket] = session;
    _allSockets.push_back(clientSocket); // select 순회 벡터에 추가

    // Player 객체 생성 및 초기화
    Player* clientPlayer = new Player();
    clientPlayer->id = clientId;
    clientPlayer->session = session;
    clientPlayer->isAlive = true;
    clientPlayer->x = dfRANGE_MOVE_LEFT + rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT);
    clientPlayer->y = dfRANGE_MOVE_TOP + rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP);
    clientPlayer->hp = 100;
    clientPlayer->direction = 4;
    clientPlayer->state = dfIdle;
    clientId++;

    // 섹터 정보 초기화
    clientPlayer->curSector.x = clientPlayer->x / SECTOR_SIZE;
    clientPlayer->curSector.y = clientPlayer->y / SECTOR_SIZE;

    // oldSector는 (-1, -1)로 설정 (처음 접속한 상태)
    clientPlayer->oldSector.x = -1;
    clientPlayer->oldSector.y = -1;


    // Contents 등록
    CGameContents* contents = dynamic_cast<CGameContents*>(_stub);
    if (contents)
        contents->AddPlayer(clientPlayer);


    // 내 캐릭터 생성 패킷 전송
    CPacket* packet = _packetPool.Alloc();
    packet->Clear();
    _proxy->mpCreateMyCharacter(clientPlayer, packet,
        clientPlayer->id, clientPlayer->direction, clientPlayer->x, clientPlayer->y, clientPlayer->hp);
    _packetPool.Free(packet);


    // 다른 클라이언트의 캐릭터 생성 패킷 전송
    // 섹터에 추가
    _sectorManager->Move(clientPlayer, clientPlayer->oldSector, clientPlayer->curSector);

    // 내 주변 섹터 목록 가져오기
    SectorAround around = _sectorManager->GetSectorAround(clientPlayer->curSector.x, clientPlayer->curSector.y);

    // 주변 플레이어 조회
    for (int i = 0; i < around.cnt; ++i)
    {
        int sx = around.around[i].x;
        int sy = around.around[i].y;

        const auto& list = _sectorManager->GetPlayerInSector(sx, sy);
        for (Player* target : list)
        {
            // 자기 자신 제외
            if (target == clientPlayer)
                continue;

            // 주변 캐릭터 생성 메시지 전송
            CPacket* packet1 = _packetPool.Alloc();
            packet1->Clear();
            _proxy->mpCreateOtherCharacter(clientPlayer, packet1, target->id, target->direction, target->x, target->y, target->hp);
            _packetPool.Free(packet1);

            // 주변 캐릭터 액션 메시지 전송
            if (target->state == dfMove)
            {
                CPacket* packet2 = _packetPool.Alloc();
                packet2->Clear();
                _proxy->mpMoveStart(clientPlayer, packet2, target->id, target->moveDir, target->x, target->y);
                _packetPool.Free(packet2);
            }

        }
    }

    // 주변 유저에게 본인 생성 메시지 전송
    CPacket* packet3 = _packetPool.Alloc();
    packet3->Clear();
    _proxy->mpCreateOtherCharacterAround(clientPlayer, packet3, clientPlayer->id,
        clientPlayer->direction, clientPlayer->x, clientPlayer->y, clientPlayer->hp);
    _packetPool.Free(packet3);

    return true;
}

//---------------------------------------------------------------------
// [RecvProc]
// - 수신 데이터 처리 및 Stub 호출
//---------------------------------------------------------------------
void CNetworkUtils::RecvProc(stSession* session)
{
    int GLE = 0;
    int retrecv = 0;
    int retenq = 0;
    int retpeekHeader = 0;
    int retpeekPayroad = 0;

    Player* player = dynamic_cast<CGameContents*>(_stub)->GetPlayer(session->sessionId);

    retrecv = recv(session->socket, session->recvQ.GetRearBufferPtr(),
        session->recvQ.DirectEnqueueSize(), 0);

    if (retrecv == SOCKET_ERROR)
    {
        GLE = WSAGetLastError();
        if (GLE == WSAEWOULDBLOCK)
        {
            return;
        }
        else if (GLE == WSAECONNRESET) // 클라 RST
        {
            // Disconnect()
            DisconnectPlayer(player);
            return;
        }
        else
        {
            _LOG(0, L"[NetworkUtils::RecvProc] recv() error: %d", GLE);
            DebugBreak();
            return;
        }
    }
    else if (retrecv == 0)
    {
        // Disconnect()
        DisconnectPlayer(player);
        return;
    }
    else if (retrecv > 0) // recv 성공
    {
        // 성공 후 _Rear 옮기기
        session->recvQ.MoveRear(retrecv);
 
        // 메시지 처리
        int recvCnt = 0;
        while (session->recvQ.GetUseSize() > sizeof(HEADER) && recvCnt++ < 3)
        {
            HEADER header = { 0 };
            CPacket* payroadPacket = _packetPool.Alloc();
            payroadPacket->Clear();

            // 헤더 복사
            retpeekHeader = session->recvQ.Peek((char*)&header, sizeof(HEADER));
            if (retpeekHeader < sizeof(HEADER))
            {
                _LOG(0, L"[NetworkUtils::RecvProc] recv ringbuffer header peek error");
                DebugBreak();
                break;
            }

            if (session->recvQ.GetUseSize() < sizeof(HEADER) + header.bySize)
            {
                break;
            }

            // 메시지 코드 확인
            if (header.byCode != 0x89)
            {
                _LOG(0, L"[NetworkUtils::RecvProc] header code error");
                DisconnectPlayer(player);
                break;
            }

            // 헤더 제거
            session->recvQ.MoveFront(sizeof(HEADER));

            // 페이로드 추출
            retpeekPayroad = session->recvQ.Peek(payroadPacket->GetBufferPtr(), header.bySize);
            if (retpeekPayroad < header.bySize)
            {
                _LOG(0, L"[NetworkUtils::RecvProc] recv ringbuffer payroad peek error");
                DebugBreak();
                break;
            }
            payroadPacket->MoveWritePos(retpeekPayroad);

            // 페이로드 제거
            session->recvQ.MoveFront(header.bySize);

            _stub->MessageProc(player, header.byType, payroadPacket);
            _packetPool.Free(payroadPacket);
        }

    }
}

size_t CNetworkUtils::GetSessionCnt()
{
    return _mapSession.size();
}

int CNetworkUtils::GetPacketPoolCapacity()
{
    return _packetPool.GetCapacityCount();
}

int CNetworkUtils::GetPacketPoolUseCount()
{
    return _packetPool.GetUseCount();
}

int CNetworkUtils::GetSessionPoolCapacity()
{
    return _sessionPool.GetCapacityCount();
}

int CNetworkUtils::GetSessionPoolUseCount()
{
    return _sessionPool.GetUseCount();
}

