#ifndef GENERATED_FUNCTIONS_H
#define GENERATED_FUNCTIONS_H

#define CODE 0x89

class CNetworkUtils; // Àü¹æ ¼±¾ð

class CGameProxy
{
public:
	void mpMoveStart(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y);
	void mpMoveStartAround(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, bool sendMe = false);
	void mpMoveStop(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y);
	void mpMoveStopAround(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, bool sendMe = false);
	void mpCreateMyCharacter(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp);
	void mpCreateMyCharacterAround(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp, bool sendMe = false);
	void mpCreateOtherCharacter(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp);
	void mpCreateOtherCharacterAround(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp, bool sendMe = false);
	void mpDeleteCharacter(Player* player, CPacket* packet, DWORD id);
	void mpDeleteCharacterAround(Player* player, CPacket* packet, DWORD id, bool sendMe = false);
	void mpAttack1(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y);
	void mpAttack1Around(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, bool sendMe = false);
	void mpAttack2(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y);
	void mpAttack2Around(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, bool sendMe = false);
	void mpAttack3(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y);
	void mpAttack3Around(Player* player, CPacket* packet, DWORD id, BYTE dir, WORD x, WORD y, bool sendMe = false);
	void mpDamage(Player* player, CPacket* packet, DWORD attackID, DWORD damageID, BYTE damageHP);
	void mpDamageAround(Player* player, CPacket* packet, DWORD attackID, DWORD damageID, BYTE damageHP, bool sendMe = false);

	void mpSync(Player* player, CPacket* packet, DWORD id, WORD x, WORD y);
	void mpSyncAround(Player* player, CPacket* packet, DWORD id, WORD x, WORD y, bool sendMe = false);
	void mpEcho(Player* player, CPacket* packet, DWORD time);
	void mpEchoAround(Player* player, CPacket* packet, DWORD time, bool sendMe = false);

	void AttackNetwork(CNetworkUtils* network);
private:
	CNetworkUtils* _netUtils = nullptr;
};


#endif GENERATED_FUNCTIONS_H
