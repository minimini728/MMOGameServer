#include <windows.h>
#include <iostream>
#include "RingBuffer.h"
#include "Structs.h"
#include "PacketBuffer.h"
#include "MemoryPool.h"

#include "IStub.h"
#include "GameStub.h"

bool CGameStub::MessageProc(Player* player, BYTE type, CPacket* payroad)
{
	switch (type)
	{
	case 10:
		return MsgProcMoveStart(player, payroad);
		break;
	case 12:
		return MsgProcMoveStop(player, payroad);
		break;
	case 20:
		return MsgProcAttack1(player, payroad);
		break;
	case 22:
		return MsgProcAttack2(player, payroad);
		break;
	case 24:
		return MsgProcAttack3(player, payroad);
		break;
	case 252:
		return MsgEcho(player, payroad);
		break;
	}
	return TRUE;
}

bool CGameStub::MsgProcMoveStart(Player*, CPacket*) { return false; }
bool CGameStub::MsgProcMoveStop(Player*, CPacket*) { return false; }
bool CGameStub::MsgProcAttack1(Player*, CPacket*) { return false; }
bool CGameStub::MsgProcAttack2(Player*, CPacket*) { return false; }
bool CGameStub::MsgProcAttack3(Player*, CPacket*) { return false; }
bool CGameStub::MsgEcho(Player*, CPacket*) { return false; }
