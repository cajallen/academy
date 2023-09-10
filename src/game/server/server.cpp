#include "server.hpp"

#include <algorithm>
#include <filesystem>
#include <chrono>
#include <thread>

#include <tracy/Tracy.hpp>

#include "extension/fmt.hpp"
#include "general/vector.hpp"
#include "general/string.hpp"
#include "general/logger.hpp"
#include "general/input.hpp"
#include "game/terminal_console.hpp"

namespace fs = std::filesystem;

namespace spellbook {

static vector<ENetPeer*> peers;

void Server::startup() {
	enet_initialize();

	ENetAddress hostAddress = {};
	hostAddress.host = ENET_HOST_ANY;
	hostAddress.port = 1234;
	host = enet_host_create(&hostAddress, 8, 2, 0, 0);

	spellbook::log(BasicMessage{fmt_("Listening on port {}", hostAddress.port)});
}

void Server::run() {
    bool want_exit = false;
    while (!want_exit) {
        want_exit = step();
    }
    
}

bool Server::step() {
    Terminal::handle_message_queue();

    bool want_exit = server_step();

    FrameMark;

    return want_exit;
}

void Server::shutdown() {
	enet_host_destroy(host);
	enet_deinitialize();
}

bool Server::server_step() {
    if (host == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        spellbook::log(BasicMessage{fmt_("No host")});
        return false;    
    }

    ENetEvent evt = {};
    if (enet_host_service(host, &evt, 1000) > 0) {
        char ip[40];
        enet_address_get_host_ip(&evt.peer->address, ip, 40);

        if (evt.type == ENET_EVENT_TYPE_CONNECT) {
            spellbook::log(BasicMessage{fmt_("New connection from {}", ip)});

            b.reset();
            b.write(PeerCommand_PeerList);
            b.write(uint32(peers.size()));
            for (auto peer : peers) {
                b.write(peer->address.host);
                b.write(peer->address.port);
            }
            ENetPacket* packetPeerList = enet_packet_create(b.buffer.data(), b.cursor, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(evt.peer, 0, packetPeerList);

            b.reset();
            b.write(PeerCommand_NewPeer);
            b.write(evt.peer->address.host);
            b.write(evt.peer->address.port);
            ENetPacket* packetNewPeer = enet_packet_create(b.buffer.data(), b.cursor, ENET_PACKET_FLAG_RELIABLE);
            for (auto peer : peers) {
                enet_peer_send(peer, 0, packetNewPeer);
            }

            peers.emplace_back(evt.peer);

        } else if (evt.type == ENET_EVENT_TYPE_DISCONNECT) {
            spellbook::log(BasicMessage{fmt_("Connection closed from {}", ip)});
            auto it = std::find(peers.begin(), peers.end(), evt.peer);
            peers.remove(it);

        } else if (evt.type == ENET_EVENT_TYPE_RECEIVE) {
            spellbook::log(BasicMessage{fmt_("Data received! {} bytes from {}", evt.packet->dataLength, ip)});

        } else {
            spellbook::log(BasicMessage{fmt_("Unknown event from {}", ip)});
        }
    } else {
        spellbook::log(BasicMessage{fmt_("... ({} peers)", peers.size())});
    }

    return false;
}

}