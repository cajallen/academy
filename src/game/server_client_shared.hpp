#pragma once

#include "general/vector.hpp"

namespace spellbook {

enum PeerCommand : uint8 {
	PeerCommand_None,
	PeerCommand_PeerList,
	PeerCommand_NewPeer,
	PeerCommand_Ping,
	PeerCommand_Pong,
};

struct QuickBuffer {
	vector<uint8> buffer;
	uint64 max_size;
	uint64 cursor;

	QuickBuffer(uint64 max_size = 0x1000) {
		buffer.resize(max_size);
		max_size = max_size;
		cursor = 0;
	}


	template<typename T>
	void write(const T& value) {
		memcpy(buffer.data() + cursor, &value, sizeof(T));
		cursor += sizeof(T);
	}

	void reset() {
		cursor = 0;
	}
};

struct QuickReadBuffer {
	vector<uint8> buffer;
	uint64 size;
	uint64 cursor;

	QuickReadBuffer(const uint8* data, uint64 size) {
		buffer = vector<uint8>(data, data + size);
		size = size;
		cursor = 0;
	}

	template<typename T>
	T read() {
		T* ret = (T*)(buffer.data() + cursor);
		cursor += sizeof(T);
		return *ret;
	}
};

}