#pragma once

#include <memory>
using std::unique_ptr;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <array>
using std::array;

#include <initializer_list>
using std::initializer_list;

#include <iostream>
using std::cout;

namespace diorama {

class noncopyable {
protected:
    noncopyable() = default;
    ~noncopyable() = default;

    noncopyable(noncopyable const &) = delete;
    void operator=(noncopyable const &) = delete;
};

}  // namespace
