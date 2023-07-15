#pragma once

#include <cstdio>

#include "general/logger.hpp"

namespace spellbook {

namespace Terminal {

inline void handle_message_queue() {
    while (!message_queue.empty()) {
        BasicMessage message = message_queue.front();
        message_queue.pop();
        if (message.frame_tags.empty())
            printf("%s", fmt_("{}: {}\n", message.group, message.str).c_str());
    }
}

}


}