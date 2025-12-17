#pragma once
class IStub
{
public:
    virtual ~IStub() {}
    virtual bool MessageProc(Player* player, BYTE type, CPacket* payload) = 0;
};
