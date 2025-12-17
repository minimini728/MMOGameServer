//=====================================================================
//  [클래스 개요]
//  CNetworkUtils
//   - 서버의 네트워크 계층을 담당하는 핵심 클래스
//=====================================================================
//  [주요 기능]
//   1) AcceptProc()  : 새로운 클라이언트 접속 처리 및 Player 생성
//   2) RecvProc()    : 클라이언트로부터 수신된 패킷 처리
//   3) UpdateNetwork(): 전체 세션의 송수신 이벤트 관리
//   4) SendUnicast() : 단일 대상 송신
//   5) SendAround()  : 주변 섹터 대상 송신
//   6) RemovePlayer(): 끊긴 세션 및 Player 정리
//=====================================================================

#pragma once

class CSectorManager;

class CNetworkUtils
{
private:
	//-----------------------------------------------------------------
	// [멤버 변수]
	//-----------------------------------------------------------------
	IStub* _stub = nullptr;						// 게임 로직 모듈
	CGameProxy* _proxy = nullptr;				// 클라로 송신 패킷 구성 담당
	CSectorManager* _sectorManager = nullptr;	// 섹터 기반 시야 관리 객체

	unordered_map<SOCKET, stSession*> _mapSession; // <소켓, 세션> 매핑 테이블
	CMemoryPool<stSession> _sessionPool;		// 세션 객체 메모리 풀
	CMemoryPool<CPacket> _packetPool;			// 패킷 객체 메모리 풀

	vector<SOCKET> _allSockets;		// select() 감시 대상 소켓 목록
	vector<SOCKET> _removeSockets;	// 종료 대상 소켓 목록


public:
	// 함수 선언
	CNetworkUtils();

	//-----------------------------------------------------------------
	// [세션 및 플레이어 관리]
	//-----------------------------------------------------------------
	void RemovePlayer();
	void DisconnectPlayer(Player* player);

	//-----------------------------------------------------------------
	// [패킷 송신 관련]
	//-----------------------------------------------------------------
	void SendUnicast(Player* player, char* msg, int size);
	void SendAround(Player* player, char* msg, int size, bool sendMe = false);

	//-----------------------------------------------------------------
	// [모듈 의존성 주입]
	//-----------------------------------------------------------------
	void AttachStub(IStub* stub);
	void AttachProxy(CGameProxy* proxy);
	void AttachSectorManager(CSectorManager* sectorManager);

	//-----------------------------------------------------------------
	// [네트워크 I/O 처리]
	//-----------------------------------------------------------------
	bool UpdateNetwork(SOCKET listen_sock);
	bool AcceptProc(SOCKET listen_sock);
	void RecvProc(stSession* session);

	//-----------------------------------------------------------------
	// [상태 조회]
	//-----------------------------------------------------------------
	size_t GetSessionCnt();
	int GetPacketPoolCapacity();
	int GetPacketPoolUseCount();
	int GetSessionPoolCapacity();
	int GetSessionPoolUseCount();
};
