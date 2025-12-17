#ifndef GENERATE_STUB_H
#define GENERATE_STUB_H

#include "IStub.h"
#define CODE 0x89

class CGameStub: public IStub
{
	bool MessageProc(Player* player, BYTE type, CPacket* payroad);
	virtual bool MsgProcMoveStart(Player*, CPacket*);
	virtual bool MsgProcMoveStop(Player*, CPacket*);
	virtual bool MsgProcAttack1(Player*, CPacket*);
	virtual bool MsgProcAttack2(Player*, CPacket*);
	virtual bool MsgProcAttack3(Player*, CPacket*);
	virtual bool MsgEcho(Player*, CPacket*);
};

#endif GENERATE_STUB_H
