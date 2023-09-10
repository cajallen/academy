#pragma once

#include "general/file/file_path.hpp"

#include "samplers.hpp"

namespace spellbook {

struct Image {
    FilePath texture;
    Sampler sampler;
};

}