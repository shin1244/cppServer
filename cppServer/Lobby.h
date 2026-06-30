#pragma once

class Lobby {
public:
	void Init();
	void HandlePacket(RecvPacket& packet);
}