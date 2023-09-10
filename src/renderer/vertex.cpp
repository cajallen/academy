#include "vertex.hpp"


namespace spellbook {
vuk::Packed Vertex::get_format() {
    return vuk::Packed {
        vuk::Format::eR32G32B32Sfloat, // position
        vuk::Format::eR32G32B32Sfloat, // normal
        vuk::Format::eR32G32B32Sfloat, // tangent
        vuk::Format::eR32G32B32Sfloat, // color
        vuk::Format::eR32G32Sfloat     // uv
    };
}

vuk::Packed Vertex::get_widget_format() {
    return vuk::Packed {
        vuk::Format::eR32G32B32Sfloat, // position
        vuk::Ignore{sizeof(Vertex::normal) + sizeof(Vertex::tangent)}, // normal
        vuk::Format::eR32G32B32Sfloat, // color
        vuk::Ignore{sizeof(Vertex::uv)}
    };
}

}
