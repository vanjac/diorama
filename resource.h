#pragma once
#include "common.h"

namespace diorama {

// An object which is owned by the World and shared by various Components within
// the World.
struct Resource {
    virtual ~Resource() = default;
};

}  // namespace