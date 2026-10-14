#pragma once

#include "blockwater/wire.hpp"
#include <string>

namespace blockwater {

class Mapping {
public:
    enum class Mode { Create, Open };
    Mapping(const std::string& name, Mode mode);
    ~Mapping();
    Mapping(const Mapping&) = delete;
    Mapping& operator=(const Mapping&) = delete;
    Mapping(Mapping&&) = delete;
    Mapping& operator=(Mapping&&) = delete;
    SharedBridge& bridge() const { return *shared_; }
private:
    std::string name_;
    bool owner_ = false;
    SharedBridge* shared_ = nullptr;
#ifdef _WIN32
    void* handle_ = nullptr;
#else
    int fd_ = -1;
#endif
};

} // namespace blockwater
