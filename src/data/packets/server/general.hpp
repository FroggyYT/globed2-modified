#pragma once
#include <data/packets/packet.hpp>
#include <data/types/gd.hpp>
#include <data/types/misc.hpp>

class GlobalPlayerListPacket : public Packet {
    GLOBED_PACKET(21000, false, false)

    GLOBED_PACKET_DECODE { buf.readValueVectorInto<PlayerPreviewAccountData>(data); }

    std::vector<PlayerPreviewAccountData> data;
};

class RoomCreatedPacket : public Packet {
    GLOBED_PACKET(21001, false, false)

    GLOBED_PACKET_DECODE { roomId = buf.readU32(); }

    uint32_t roomId;
};

class RoomJoinedPacket : public Packet {
    GLOBED_PACKET(21002, false, false)
    GLOBED_PACKET_DECODE {}
};

class RoomJoinFailedPacket : public Packet {
    GLOBED_PACKET(21003, false, false)
    GLOBED_PACKET_DECODE {}
};

class RoomPlayerListPacket : public Packet {
    GLOBED_PACKET(21004, false, false)

    GLOBED_PACKET_DECODE {
        roomId = buf.readU32();
        buf.readValueVectorInto<PlayerRoomPreviewAccountData>(data);
    }

    uint32_t roomId;
    std::vector<PlayerRoomPreviewAccountData> data;
};

class LevelListPacket : public Packet {
    GLOBED_PACKET(21005, false, false)

    GLOBED_PACKET_DECODE {
        buf.readValueVectorInto<GlobedLevel>(levels);
    }

    std::vector<GlobedLevel> levels;
};
