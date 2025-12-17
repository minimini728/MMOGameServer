#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <iostream>
#include <ws2tcpip.h>

#include <psapi.h>
#include <DbgHelp.h>
#include "CCrashDump.h"
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "Psapi.lib")

#include "RingBuffer.h"
#include <list>
#include <unordered_map>
#include "Structs.h"
#include "SectorManager.h"
#include "PacketBuffer.h"
#include "MemoryPool.h"

#include "IStub.h"
#include "GameStub.h"
#include "GameContents.h"
#include "GameProxy.h"
#include "NetworkUtils.h"
#pragma comment(lib, "winmm.lib")
#include <ctime>
#include "LogManager.h"
#include "Profiler.h"

using namespace std;

#define SERVERPORT 20901
#define MAX 160
#define FRAME_TIME 20

//---------------------------------------------------------------
// [전역 변수]
//---------------------------------------------------------------
int old_tick = 0;
int g_loopCnt = 0;

DWORD g_networkProcTime = 0;
DWORD g_logicTime = 0;

CCrashDump dump; // 자동 덤프 생성기

bool g_get = false; // 수동 프로파일링 플래그

//---------------------------------------------------------------
// [함수 선언]
//---------------------------------------------------------------
void FPS();
void PrintStatus();

int main()
{
    timeBeginPeriod(1);

    cout << "[main] main start" << endl;
    srand((unsigned int)time(NULL)); // 랜덤 시드 초기화
    int GLE; // GetLastError
    int retbind; // bind() return
    int retlisten; // listen() return
    int retioct; // ioct() return
    int retsetsockopt; // setsockopt() return

    // 주요 모듈 생성
    CGameContents* contents = new CGameContents();
    CNetworkUtils* network = new CNetworkUtils();
    CGameProxy* proxy = new CGameProxy();
    CSectorManager* sectorManager = new CSectorManager();

    // 모듈 간 연결
    contents->SetNetworkUtils(network);
    contents->SetProxy(proxy);
    contents->SetSectorManager(sectorManager);

    network->AttachStub(contents);
    network->AttachProxy(proxy);
    network->AttachSectorManager(sectorManager);

    proxy->AttackNetwork(network);


    // 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        GLE = WSAGetLastError();
        cout << "WSAStartup() error" << GLE << endl;
        return 0;
    }

    // socket()
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET)
    {
        GLE = WSAGetLastError();
        cout << "socket() error" << GLE << endl;
        return 0;
    }

    // bind()
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    retbind = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retbind == SOCKET_ERROR)
    {
        GLE = WSAGetLastError();
        cout << "bind() error" << GLE << endl;
        return 0;
    }

    // listen()
    retlisten = listen(listen_sock, SOMAXCONN_HINT(0x7fffffff));
    if (retlisten == SOCKET_ERROR)
    {
        GLE = WSAGetLastError();
        cout << "listen() error" << GLE << endl;
        return 0;
    }

    // 논블로킹 소켓으로 전환
    u_long on = 1;
    retioct = ioctlsocket(listen_sock, FIONBIO, &on);
    if (retioct == SOCKET_ERROR)
    {
        GLE = WSAGetLastError();
        cout << "ioctlsocket() error" << GLE << endl;
        return 0;
    }

    // Linger 걸기
    LINGER optval;
    optval.l_onoff = 1;
    optval.l_linger = 0;
    retsetsockopt = setsockopt(listen_sock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
    if (retsetsockopt == SOCKET_ERROR)
    {
        GLE = WSAGetLastError();
        printf("setsockopt Linger(): %d", GLE);
        return 1;
    }

    int previousTime = timeGetTime();
    old_tick = previousTime;
    DWORD netBeginTime;
    DWORD logicBeginTime;

    // 게임 루프
    while (1)
    {
        // --------------- 네트워크 처리 ------------------
        netBeginTime = timeGetTime();
        ProfileBegin("network");
        if (!network->UpdateNetwork(listen_sock))
        {
            _LOG(0, L"[main] network return false");
            break;
        }
        ProfileEnd("network");
        g_networkProcTime += timeGetTime() - netBeginTime;


        // --------------- 게임 로직 처리 ------------------
        logicBeginTime = timeGetTime();
        ProfileBegin("contents");
        if (!contents->UpdateGame())
        {
            _LOG(0, L"[main] contents return false");
            break;
        }
        ProfileEnd("contents");
        g_logicTime += timeGetTime() - logicBeginTime;


        // --------------- 세션 정리 --------------------
        ProfileBegin("RemovePlayer");
        network->RemovePlayer();
        ProfileEnd("RemovePlayer");


        // ----------------------- 디버깅용 단축키 ------------------------
        if (GetAsyncKeyState('E') & 0x0001) ProfileDataOut(); // 프로파일 출력
        if (GetAsyncKeyState('G') & 0x0001) g_get = true;     // 강제 FPS 출력
        if (GetAsyncKeyState('R') & 0x0001) ProfileReset();   // 프로파일 초기화


        // -------------- FPS 계산 -------------------
        FPS();


        // ------------- 프레임 보정 -------------------
        int elapse = timeGetTime() - previousTime;
        int sleeptick = FRAME_TIME - elapse;

        if (sleeptick > 0)
        {
            Sleep(sleeptick);
        }
        previousTime += FRAME_TIME;

        g_loopCnt++;

    }

    _LOG(0, L"[main] return");

    timeEndPeriod(1);
}

// -------------------------------------------------
// 1초마다 루프 초기화 및 루프 지연시 출력
// -------------------------------------------------
void FPS()
{
    if (timeGetTime() - old_tick >= 1000)
    {
        if (g_get || g_loopCnt < 46)
        {
            ProfileBegin("FPS");
            PrintStatus();
            ProfileEnd("FPS");

            g_get = false;

        }
        old_tick += 1000;

        // 누적값 초기화
        g_loopCnt = 0;
        g_networkProcTime = 0;
        g_logicTime = 0;

    }
}

// -------------------------------------------------
// 콘솔 및 로그에 서버 성능 상태 출력
// -------------------------------------------------
void PrintStatus()
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t buffer[512];
    swprintf_s(buffer,
        L"[%02d/%02d/%02d %02d:%02d:%02d]\n"
        L"-----------------------------------\n"
        L"Loop/sec : %d\n"
        L"-----------------------------------\n"
        L"NetworkProc Time : %d ms\n"
        L"Logic Time : %d ms\n"
        L"-----------------------------------\n\n\n",
        st.wYear % 100, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond,
        g_loopCnt,
        g_networkProcTime / g_loopCnt,
        g_logicTime / g_loopCnt
    );

    // 콘솔 출력
    wprintf(L"%s", buffer);
    // 로그 파일 출력
    if (g_loopCnt < 49)
    {
        _LOG(0, buffer);
    }

}
