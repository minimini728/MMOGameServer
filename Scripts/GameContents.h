//=====================================================================
//  [클래스 개요]
//  CGameContents
//   - 서버의 게임 로직(Contents Layer)을 담당하는 클래스
//=====================================================================
//  [주요 기능]
//   1) MsgProc...()  : 클라이언트 요청 처리
//   2) UpdateGame()  : 게임 프레임 단위 업데이트
//   3) 섹터 단위 시야 관리 및 충돌 처리
//   4) Player 생성·삭제 및 메모리 관리
//=====================================================================

#pragma once

// 전방 선언
class CNetworkUtils;
class CGameProxy;
class CSectorManager;

class CGameContents : public CGameStub
{
private:
    //-----------------------------------------------------------------
    // [멤버 변수]
    //-----------------------------------------------------------------
    CNetworkUtils* _netUtils = nullptr;         // 네트워크 모듈
    CGameProxy* _proxy = nullptr;               // Proxy 객체 (클라이언트 송신)
    CSectorManager* _sectorManager = nullptr;   // 섹터 기반 시야 관리 객체
    unordered_map</*sessionId*/DWORD, Player*> _mapPlayer; // 세션ID 기반 Player 관리 컨테이너
    CMemoryPool<CPacket> _packetPool;           // 패킷 재사용을 위한 메모리풀

    //-----------------------------------------------------------------
    // [내부 로직 함수]
    //-----------------------------------------------------------------
    void MovePlayer(Player* player);            // 플레이어 이동 처리
    bool MsgProcMoveStart(Player*, CPacket*);   // 이동 시작 메시지 처리
    bool MsgProcMoveStop(Player*, CPacket*);    // 이동 종료 메시지 처리
    bool MsgProcAttack1(Player*, CPacket*);     // 공격 1 타입 메시지 처리
    bool MsgProcAttack2(Player*, CPacket*);     // 공격 2 타입 메시지 처리
    bool MsgProcAttack3(Player*, CPacket*);     // 공격 3 타입 메시지 처리
    bool MsgEcho(Player*, CPacket*);            // Echo 메시지 처리

    bool IsSameSector(const SectorPos& a, const SectorPos& b);  // 섹터 위치 비교 함수

public: 
    CGameContents();
    //-----------------------------------------------------------------
    // [모듈 의존성 주입]
    //-----------------------------------------------------------------
    void SetNetworkUtils(CNetworkUtils* netUtils);  // 네트워크 모듈 연결
    void SetProxy(CGameProxy* proxy);               // Proxy 모듈 연결
    void SetSectorManager(CSectorManager* sectorManager);   // 섹터 매니저 연결

    //-----------------------------------------------------------------
    // [플레이어 관리]
    //-----------------------------------------------------------------
    void AddPlayer(Player* player);                 // 새로운 플레이어 추가
    Player* GetPlayer(DWORD sessionId);             // 세션 ID로 Player 조회
    void RemovePlayer(DWORD sessionId);             // 세션 ID로 Player 제거
    void UpdateSight(Player* player);               // 시야 갱신 및 주변 정보 송신

    //-----------------------------------------------------------------
    // [게임 프레임 단위 업데이트]
    //-----------------------------------------------------------------
    bool UpdateGame();                              // 게임 로직 프레임 업데이트

    //-----------------------------------------------------------------
    // [상태 조회]
    //-----------------------------------------------------------------
    int GetPacketPoolCapacity();                    // 패킷 풀 총 용량
    int GetPacketPoolUseCount();                    // 패킷 풀 현재 사용량

};
