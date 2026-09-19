#pragma once

#include <string>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

namespace itch {

class MmapReader {
public:
    explicit MmapReader(const std::string& path) {
        fd_ = open(path.c_str(), O_RDONLY);
        if (fd_ < 0) {
            throw std::runtime_error("Failed to open file: " + path + " - " + std::strerror(errno));
        }

        struct stat sb;
        if (fstat(fd_, &sb) < 0) {
            close(fd_);
            throw std::runtime_error("Failed to stat file: " + path + " - " + std::strerror(errno));
        }

        size_ = sb.st_size;
        if (size_ > 0) {
            // MAP_POPULATE helps pre-fault the pages, preventing page faults during execution
            mapped_ = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd_, 0);
            if (mapped_ == MAP_FAILED) {
                close(fd_);
                throw std::runtime_error("Failed to mmap file: " + path + " - " + std::strerror(errno));
            }

            // Hint to the OS that we will read sequentially
            madvise(mapped_, size_, MADV_SEQUENTIAL);
        }
    }

    ~MmapReader() {
        if (mapped_ && mapped_ != MAP_FAILED) {
            munmap(mapped_, size_);
        }
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    MmapReader(const MmapReader&) = delete;
    MmapReader& operator=(const MmapReader&) = delete;

    MmapReader(MmapReader&& other) noexcept 
        : mapped_(other.mapped_), size_(other.size_), fd_(other.fd_) {
        other.mapped_ = nullptr;
        other.size_ = 0;
        other.fd_ = -1;
    }

    MmapReader& operator=(MmapReader&& other) noexcept {
        if (this != &other) {
            if (mapped_ && mapped_ != MAP_FAILED) {
                munmap(mapped_, size_);
            }
            if (fd_ >= 0) {
                close(fd_);
            }

            mapped_ = other.mapped_;
            size_ = other.size_;
            fd_ = other.fd_;

            other.mapped_ = nullptr;
            other.size_ = 0;
            other.fd_ = -1;
        }
        return *this;
    }

    const char* data() const { return static_cast<const char*>(mapped_); }
    size_t size() const { return size_; }

private:
    void* mapped_ = nullptr;
    size_t size_ = 0;
    int fd_ = -1;
};

} // namespace itch
