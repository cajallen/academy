#pragma once

#include <enet/enet.h>

#include "editor/gui.hpp"
#include "game/server_client_shared.hpp"

namespace spellbook {

struct Client {
    GUI      gui;
    
    string external_resource_folder;
    string resource_folder;
    string user_folder;

    ENetHost* host = nullptr;
    ENetPeer* server_peer = nullptr;

    QuickBuffer wb = {};

    void startup();
    void run();
    void step(bool skip_input);
    void shutdown();

    void client_step();
};

inline Client& get_client() {
    static Client client;
    return client;
}

}