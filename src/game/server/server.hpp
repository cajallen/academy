#pragma once

#include <enet/enet.h>

#include "general/string.hpp"
#include "game/server_client_shared.hpp"

namespace spellbook {

struct Server {
    ENetHost* host = nullptr;

    QuickBuffer b = {};

    void startup();
    void run();
    bool step();
    void shutdown();

    bool server_step();
};

inline Server& get_server() {
    static Server server;
    return server;
}

}