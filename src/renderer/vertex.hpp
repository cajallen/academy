#pragma once

#include <vuk/CommandBuffer.hpp>

#include "general/math/geometry.hpp"

namespace spellbook {

struct Vertex {
	v3 position;
	v3 normal = {};
	v3 tangent = {};
	v3 color = {};
	v2 uv = {};

    static vuk::Packed get_format();
    static vuk::Packed get_widget_format();
};

}
