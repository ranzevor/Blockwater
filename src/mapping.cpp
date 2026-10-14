#include "blockwater/mapping.hpp"
#include "blockwater/link.hpp"
#include <new>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace blockwater {

namespace {
bool valid_name(const std::string& name) {
    if (name.empty() || name.size() > 100) return false;
    for (char c : name)
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
            !(c >= '0' && c <= '9') && c != '-' && c != '_') return false;
    return true;
}
} // namespace

Mapping::Mapping(const std::string& name, Mode mode) : name_(name) {
    if (!valid_name(name)) throw std::invalid_argument("invalid mapping name");
#ifdef _WIN32
    const auto wide = std::wstring(name.begin(), name.end());
    const auto local_name = std::wstring(L"Local\\Blockwater_") + wide;
    HANDLE h = nullptr;
    if (mode == Mode::Create) {
        h = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
            0, static_cast<DWORD>(sizeof(SharedBridge)), local_name.c_str());
        if (h && GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(h);
            throw std::runtime_error("mapping already exists");
        }
        owner_ = true;
    } else {
        h = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, local_name.c_str());
    }
    if (!h) throw std::runtime_error("cannot create or open Windows mapping");
    handle_ = h;
    shared_ = static_cast<SharedBridge*>(MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBridge)));
    if (!shared_) {
        CloseHandle(h);
        handle_ = nullptr;
        throw std::runtime_error("cannot map Windows region");
    }
#else
    const std::string posix_name = "/blockwater_" + name;
    if (mode == Mode::Create) {
        fd_ = shm_open(posix_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
        if (fd_ < 0) throw std::runtime_error("cannot create POSIX mapping");
        owner_ = true;
        if (ftruncate(fd_, sizeof(SharedBridge)) != 0) {
            close(fd_);
            shm_unlink(posix_name.c_str());
            throw std::runtime_error("cannot size POSIX mapping");
        }
    } else {
        fd_ = shm_open(posix_name.c_str(), O_RDWR, 0600);
        if (fd_ < 0) throw std::runtime_error("cannot open POSIX mapping");
        struct stat info = {};
        if (fstat(fd_, &info) != 0 || info.st_size != static_cast<off_t>(sizeof(SharedBridge))) {
            close(fd_);
            throw std::runtime_error("POSIX mapping size mismatch");
        }
    }
    void* ptr = mmap(nullptr, sizeof(SharedBridge), PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (ptr == MAP_FAILED) {
        close(fd_);
        if (owner_) shm_unlink(posix_name.c_str());
        throw std::runtime_error("cannot map POSIX region");
    }
    shared_ = static_cast<SharedBridge*>(ptr);
#endif
    if (owner_) {
        new (shared_) SharedBridge{};
        initialize(*shared_);
    } else if (std::atomic_ref<std::uint64_t>(shared_->signature).load(std::memory_order_acquire) != kSignature ||
               shared_->size_bytes != sizeof(SharedBridge)) {
        // A failed opener owns no shared memory and must not unlink it.
#ifdef _WIN32
        UnmapViewOfFile(shared_);
        CloseHandle(static_cast<HANDLE>(handle_));
        handle_ = nullptr;
#else
        munmap(shared_, sizeof(SharedBridge));
        close(fd_);
        fd_ = -1;
#endif
        shared_ = nullptr;
        owner_ = false;
        throw std::runtime_error("bridge protocol mismatch");
    }
}

Mapping::~Mapping() {
#ifdef _WIN32
    if (shared_) UnmapViewOfFile(shared_);
    if (handle_) CloseHandle(static_cast<HANDLE>(handle_));
#else
    if (shared_) munmap(shared_, sizeof(SharedBridge));
    if (fd_ >= 0) close(fd_);
    if (owner_) shm_unlink(("/blockwater_" + name_).c_str());
#endif
}

} // namespace blockwater
