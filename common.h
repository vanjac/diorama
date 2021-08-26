#pragma once

#include <memory>
using std::unique_ptr;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <array>
using std::array;

#include <unordered_map>
using std::unordered_map;

#include <initializer_list>
using std::initializer_list;

namespace diorama {

class noncopyable {
protected:
    noncopyable() = default;
    ~noncopyable() = default;

    noncopyable(noncopyable const &) = delete;
    void operator=(noncopyable const &) = delete;
};

}  // namespace
