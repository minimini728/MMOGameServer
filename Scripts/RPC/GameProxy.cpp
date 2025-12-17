#include <windows.h>
#include <iostream>
#include "RingBuffer.h"
#include <list>
#include <unordered_map>
#include "Structs.h"
#include "SectorManager.h"
#include "PacketBuffer.h"
#include "MemoryPool.h"

#include "IStub.h"
#include "GameProxy.h"
#include "GameStub.h"
#include "GameContents.h"
#include "NetworkUtils.h"

void CGameProxy::AttackNetwork(CNetworkUtils* network)
{
	_netUtils = network;
}

void CGameProxy::mpMoveStart(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
	*packet << (BYTE)CODE << size << (BYTE)11 << id << dir << x << y;

	_netUtils->SendUnicast(player, packet->GetBufferPtr(), packet->GetDataSize());
}

void CGameProxy::mpMoveStartAround(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, bool sendMe)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
	*packet << (BYTE)CODE << size << (BYTE)11 << id << dir << x << y;

	_netUtils->SendAround(player, packet->GetBufferPtr(), packet->GetDataSize(), sendMe);
}


void CGameProxy::mpMoveStop(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
	*packet << (BYTE)CODE << size << (BYTE)13 << id << dir << x << y;

	_netUtils->SendUnicast(player, packet->GetBufferPtr(), packet->GetDataSize());
}

void CGameProxy::mpMoveStopAround(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, bool sendMe)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
	*packet << (BYTE)CODE << size << (BYTE)13 << id << dir << x << y;

	_netUtils->SendAround(player, packet->GetBufferPtr(), packet->GetDataSize(), sendMe);
}


void CGameProxy::mpCreateMyCharacter(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y) + sizeof(hp);
	*packet << (BYTE)CODE << size << (BYTE)0 << id << dir << x << y << hp;

	_netUtils->SendUnicast(player, packet->GetBufferPtr(), packet->GetDataSize());
}

void CGameProxy::mpCreateMyCharacterAround(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp, bool sendMe)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y) + sizeof(hp);
	*packet << (BYTE)CODE << size << (BYTE)0 << id << dir << x << y << hp;

	_netUtils->SendAround(player, packet->GetBufferPtr(), packet->GetDataSize(), sendMe);
}


void CGameProxy::mpCreateOtherCharacter(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y) + sizeof(hp);
	*packet << (BYTE)CODE << size << (BYTE)1 << id << dir << x << y << hp;

	_netUtils->SendUnicast(player, packet->GetBufferPtr(), packet->GetDataSize());
}

void CGameProxy::mpCreateOtherCharacterAround(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp, bool sendMe)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y) + sizeof(hp);
	*packet << (BYTE)CODE << size << (BYTE)1 << id << dir << x << y << hp;

	_netUtils->SendAround(player, packet->GetBufferPtr(), packet->GetDataSize(), sendMe);
}


void CGameProxy::mpDeleteCharacter(Player* player, CPacket* packet, DWORD id)
{
	BYTE size = sizeof(id);
	*packet << (BYTE)CODE << size << (BYTE)2 << id;

	_netUtils->SendUnicast(player, packet->GetBufferPtr(), packet->GetDataSize());
}

void CGameProxy::mpDeleteCharacterAround(Player* player, CPacket* packet, DWORD id, bool sendMe)
{
	BYTE size = sizeof(id);
	*packet << (BYTE)CODE << size << (BYTE)2 << id;

	_netUtils->SendAround(player, packet->GetBufferPtr(), packet->GetDataSize(), sendMe);
}


void CGameProxy::mpAttack1(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
	*packet << (BYTE)CODE << size << (BYTE)21 << id << dir << x << y;

	_netUtils->SendUnicast(player, packet->GetBufferPtr(), packet->GetDataSize());
}

void CGameProxy::mpAttack1Around(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, bool sendMe)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
	*packet << (BYTE)CODE << size << (BYTE)21 << id << dir << x << y;

	_netUtils->SendAround(player, packet->GetBufferPtr(), packet->GetDataSize(), sendMe);
}


void CGameProxy::mpAttack2(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
	*packet << (BYTE)CODE << size << (BYTE)23 << id << dir << x << y;

	_netUtils->SendUnicast(player, packet->GetBufferPtr(), packet->GetDataSize());
}

void CGameProxy::mpAttack2Around(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, bool sendMe)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
	*packet << (BYTE)CODE << size << (BYTE)23 << id << dir << x << y;

	_netUtils->SendAround(player, packet->GetBufferPtr(), packet->GetDataSize(), sendMe);
}


void CGameProxy::mpAttack3(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
	*packet << (BYTE)CODE << size << (BYTE)25 << id << dir << x << y;

	_netUtils->SendUnicast(player, packet->GetBufferPtr(), packet->GetDataSize());
}

void CGameProxy::mpAttack3Around(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, bool sendMe)
{
	BYTE size = sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
	*packet << (BYTE)CODE << size << (BYTE)25 << id << dir << x << y;

	_netUtils->SendAround(player, packet->GetBufferPtr(), packet->GetDataSize(), sendMe);
}


void CGameProxy::mpDamage(Player* player, CPacket* packet, DWORD attackID, DWORD damageID, BYTE damageHP)
{
	BYTE size = sizeof(attackID) + sizeof(damageID) + sizeof(damageHP);
	*packet << (BYTE)CODE << size << (BYTE)30 << attackID << damageID << damageHP;

	_netUtils->SendUnicast(player, packet->GetBufferPtr(), packet->GetDataSize());
}

void CGameProxy::mpDamageAround(Player* player, CPacket* packet, DWORD attackID, DWORD damageID, BYTE damageHP, bool sendMe)
{
	BYTE size = sizeof(attackID) + sizeof(damageID) + sizeof(damageHP);
	*packet << (BYTE)CODE << size << (BYTE)30 << attackID << damageID << damageHP;

	_netUtils->SendAround(player, packet->GetBufferPtr(), packet->GetDataSize(), sendMe);
}

void CGameProxy::mpSync(Player* player, CPacket* packet, DWORD id, WORD x, WORD y)
{
	BYTE size = sizeof(id) + sizeof(x) + sizeof(y);
	*packet << (BYTE)CODE << size << (BYTE)251 << id << x << y;

	_netUtils->SendUnicast(player, packet->GetBufferPtr(), packet->GetDataSize());
}

void CGameProxy::mpSyncAround(Player* player, CPacket* packet, DWORD id, WORD x, WORD y, bool sendMe)
{
	BYTE size = sizeof(id) + sizeof(x) + sizeof(y);
	*packet << (BYTE)CODE << size << (BYTE)251 << id << x << y;

	_netUtils->SendAround(player, packet->GetBufferPtr(), packet->GetDataSize(), sendMe);
}

void CGameProxy::mpEcho(Player* player, CPacket* packet, DWORD time)
{
	BYTE size = sizeof(time);
	*packet << (BYTE)CODE << size << (BYTE)253 << time;

	_netUtils->SendUnicast(player, packet->GetBufferPtr(), packet->GetDataSize());
}

void CGameProxy::mpEchoAround(Player* player, CPacket* packet, DWORD time, bool sendMe)
{
	BYTE size = sizeof(time);
	*packet << (BYTE)CODE << size << (BYTE)253 << time;

	_netUtils->SendAround(player, packet->GetBufferPtr(), packet->GetDataSize(), sendMe);
}


