#pragma once
#include "common.h"

namespace diorama {

// An object which is owned by the World and shared by various Components within
// the World.
class Resource {
public:
    virtual ~Resource() = default;
};

}  // namespace