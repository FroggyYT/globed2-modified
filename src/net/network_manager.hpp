#pragma once
#include "game_socket.hpp"

#include <functional>
#include <unordered_map>
#include <thread>

#include <managers/game_server.hpp>
#include <managers/central_server.hpp>
#include <util/sync.hpp>
#include <util/time.hpp>

using namespace util::sync;

enum class NetworkThreadTask {
    PingServers
};

template <typename T>
concept HasPacketID = requires { T::PACKET_ID; };

// This class is fully thread safe..? hell do i know..
class NetworkManager : GLOBED_SINGLETON(NetworkManager) {
public:
    using PacketCallback = std::function<void(std::shared_ptr<Packet>)>;

    template <HasPacketID Pty>
    using PacketCallbackSpecific = std::function<void(Pty*)>;

    static constexpr uint16_t PROTOCOL_VERSION = 1;
    static constexpr util::data::byte SERVER_MAGIC[10] = {0xda, 0xee, 'g', 'l', 'o', 'b', 'e', 'd', 0xda, 0xee};

    AtomicU32 connectedTps; // if `authenticated() == true`, this is the TPS of the current server, otherwise undefined.

    NetworkManager();
    ~NetworkManager();

    // Connect to a server
    bool connect(const std::string& addr, unsigned short port, bool standalone = false);
    // Safer version of `connect`, sets the active game server in `GameServerManager` on success, doesn't throw on exception on error
    void connectWithView(const GameServer& gsview);
    // Is similar to `connectWithView` (does not throw exceptions) but is made specifically for standalone servers.
    // Grabs the address from the first server in `GameServerManager`
    void connectStandalone();

    // Disconnect from a server. Does nothing if not connected
    void disconnect(bool quiet = false);

    // Sends a packet to the currently established connection. Throws if disconnected.
    void send(std::shared_ptr<Packet> packet);

    // Adds a packet listener and calls your callback function when a packet with `id` is received.
    // If there already was a callback with this packet ID, it gets replaced.
    // All callbacks are ran in the main (GD) thread.
    void addListener(packetid_t id, PacketCallback callback);

    // Same as addListener(packetid_t, PacketCallback) but hacky syntax xd
    template <HasPacketID Pty>
    void addListener(PacketCallbackSpecific<Pty> callback) {
        this->addListener(Pty::PACKET_ID, [callback](std::shared_ptr<Packet> pkt) {
            callback(static_cast<Pty*>(pkt.get()));
        });
    }

    // Removes a listener by packet ID.
    void removeListener(packetid_t id);

    // Same as removeListener(packetid_t) but hacky syntax once again
    template <HasPacketID T>
    void removeListener() {
        this->removeListener(T::PACKET_ID);
    }

    // Removes all listeners.
    void removeAllListeners();

    // queues task for pinging servers
    void taskPingServers();

    // Returns true if ANY connection has been made with a server. The handshake might not have been done at this point.
    bool connected();

    // Returns true ONLY if we are connected to a server and the crypto handshake has finished. We might not have logged in yet.
    bool handshaken();

    // Returns true if we have fully authenticated and are ready to rock.
    bool established();

    // Returns true if we are connected to a standalone game server, not tied to any central server.
    bool standalone();

private:
    static constexpr chrono::seconds KEEPALIVE_INTERVAL = chrono::seconds(5);
    static constexpr chrono::seconds DISCONNECT_AFTER = chrono::seconds(15);

    GameSocket gameSocket;

    SmartMessageQueue<std::shared_ptr<Packet>> packetQueue;
    SmartMessageQueue<NetworkThreadTask> taskQueue;

    WrappingMutex<std::unordered_map<packetid_t, PacketCallback>> listeners;

    // threads

    void threadMainFunc();
    void threadRecvFunc();

    std::thread threadMain;
    std::thread threadRecv;

    // misc

    AtomicBool _running = true;
    AtomicBool _handshaken = false;
    AtomicBool _loggedin = false;
    AtomicBool _connectingStandalone = false;

    util::time::time_point lastKeepalive;
    util::time::time_point lastReceivedPacket;

    void handlePingResponse(std::shared_ptr<Packet> packet);
    void maybeSendKeepalive();
    void maybeDisconnectIfDead();

    // Builtin listeners have priority above the others.
    WrappingMutex<std::unordered_map<packetid_t, PacketCallback>> builtinListeners;

    void addBuiltinListener(packetid_t id, PacketCallback callback);

    template <HasPacketID Pty>
    void addBuiltinListener(PacketCallbackSpecific<Pty> callback) {
        this->addBuiltinListener(Pty::PACKET_ID, [callback](std::shared_ptr<Packet> pkt) {
            callback(static_cast<Pty*>(pkt.get()));
        });
    }
};