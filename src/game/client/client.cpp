#include "client.hpp"

#include <algorithm>
#include <filesystem>

#include <tracy/Tracy.hpp>

#include "extension/fmt.hpp"
#include "general/vector.hpp"
#include "general/string.hpp"
#include "general/logger.hpp"
#include "renderer/renderer.hpp"
#include "editor/console.hpp"
#include "editor/editor_scene.hpp"
#include "game/input.hpp"

constexpr string_view ServerHostName = "example.com";

namespace fs = std::filesystem;

namespace spellbook {

static vector<ENetPeer*> remote_peers = {};

void Client::startup() {
    external_resource_folder = (fs::current_path() / "external_resources").string();
    resource_folder = (fs::current_path() / "resources").string();
    user_folder = (fs::current_path() / "user").string();

    Console::setup();
    get_renderer().setup();
    Input::setup();

    for (auto& editor_scene : get_editor_scenes()) {
        editor_scene->setup();
    }
    gui.setup();


    enet_initialize();

	ENetAddress host_address = {};
	host_address.host = ENET_HOST_ANY;
	host_address.port = ENET_PORT_ANY;
	host = enet_host_create(&host_address, 2, 2, 0, 0);

    assert_else(host != nullptr);

	ENetAddress server_address = {};
	enet_address_set_host(&server_address, ServerHostName.data());
	server_address.port = 1234;
	server_peer = enet_host_connect(host, &server_address, 2, 0);
}

void Client::run() {
    while (!Input::exit_accepted) {
        step(false);
    }
}

void Client::step(bool skip_input) {
    if (!skip_input)
        Input::update();
    get_renderer().update();
    gui.update();

    client_step();
    
    for (auto& editor_scene : get_editor_scenes()) {
        editor_scene->update();
    }
    get_renderer().render();
    FrameMark;
}

void Client::client_step() {
    ENetEvent evt = {};
    if (enet_host_service(host, &evt, 100) > 0) {
        char ip[40];
        enet_address_get_host_ip(&evt.peer->address, ip, 40);

        if (evt.type == ENET_EVENT_TYPE_CONNECT) {
            spellbook::log(BasicMessage{fmt_("Connected to {}:{}", ip, int32(evt.peer->address.port))});
        } else if (evt.type == ENET_EVENT_TYPE_DISCONNECT) {
            spellbook::log(BasicMessage{fmt_("Disconnected from {}:{}", ip, int32(evt.peer->address.port))});

            auto it = std::find(remote_peers.begin(), remote_peers.end(), evt.peer);
            if (it != remote_peers.end()) {
                remote_peers.remove(it);
            }
        } else if (evt.type == ENET_EVENT_TYPE_RECEIVE) {
            QuickReadBuffer b((const uint8*)evt.packet->data, evt.packet->dataLength);
            PeerCommand pc = b.read<PeerCommand>();
            if (pc == PeerCommand_PeerList) {
                uint32 numPeers = b.read<uint32>();
                spellbook::log(BasicMessage{fmt_("{} peers from {}", int32(numPeers), ip)});
                for (uint32 i = 0; i < numPeers; i++) {
                    ENetAddress addr = {};
                    addr.host = b.read<in6_addr>();
                    addr.port = b.read<uint16>();

                    char peerListIp[40];
                    enet_address_get_host_ip(&addr, peerListIp, 40);

                    spellbook::log(BasicMessage{fmt_("- {}:{}", peerListIp, int32(addr.port))});

                    ENetPeer* peerRemote = enet_host_connect(host, &addr, 2, 0);
                    remote_peers.emplace_back(peerRemote);
                }
            } else if (pc == PeerCommand_NewPeer) {
                ENetAddress addr = {};
                addr.host = b.read<in6_addr>();
                addr.port = b.read<uint16>();

                char newPeerIp[40];
                enet_address_get_host_ip(&addr, newPeerIp, 40);

                spellbook::log(BasicMessage{fmt_("New peer connected from {}: {}:{}", ip, newPeerIp, int32(addr.port))});

                ENetPeer* peerRemote = enet_host_connect(host, &addr, 2, 0);
                remote_peers.emplace_back(peerRemote);

            } else if (pc == PeerCommand_Ping) {
                spellbook::log(BasicMessage{fmt_("Received ping from {}:{}", ip, int32(evt.peer->address.port))});

                wb.reset();
                wb.write(PeerCommand_Pong);
                ENetPacket* packetPong = enet_packet_create(wb.buffer.data(), wb.cursor, ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(evt.peer, 0, packetPong);
            } else if (pc == PeerCommand_Pong) {
                spellbook::log(BasicMessage{fmt_("Received pong from {}:{}", ip, int32(evt.peer->address.port))});
            } else {
                spellbook::log(BasicMessage{fmt_("Unknown peer command {} from {}", int32(pc), ip)});
            }

            enet_packet_destroy(evt.packet);

        } else {
            spellbook::log(BasicMessage{fmt_("Unknown event from {}", ip)});
        }
    } else {
        spellbook::log(BasicMessage{fmt_("... ({} remote peers)", int32(remote_peers.size()))});
        for (auto peer : remote_peers) {
            wb.reset();
            wb.write(PeerCommand_Ping);
            ENetPacket* packetPing = enet_packet_create(wb.buffer.data(), wb.cursor, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(peer, 0, packetPing);
        }
    }
}

void Client::shutdown() {
	enet_host_destroy(host);
	enet_deinitialize();
}

}
