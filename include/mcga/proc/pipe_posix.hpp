#pragma once

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <memory>
#include <stdexcept>
#include <system_error>

namespace mcga::proc::internal {

class PosixPipeReader : public PipeReader {
  private:
    static constexpr std::size_t kBlockReadSize = 128;

  public:
    explicit PosixPipeReader(const int& inputFD)
            : inputFD(inputFD),
              buffer(static_cast<std::uint8_t*>(malloc(kBlockReadSize))),
              bufferReadHead(0), bufferSize(0), bufferCapacity(kBlockReadSize) {
    }

    ~PosixPipeReader() override {
        close(inputFD);
        free(buffer);
    }

    Message getNextMessage(int maxConsecutiveFailedReadAttempts) override {
        // Try reading a message first, maybe we received multiple at once.
        auto message = readMessageFromBuffer();
        if (!message.isInvalid()) {
            return message;
        }

        int failedAttempts = 0;
        while (maxConsecutiveFailedReadAttempts == -1
               || failedAttempts <= maxConsecutiveFailedReadAttempts) {
            bool successful = readBytes();
            if (!successful) {
                failedAttempts += 1;
                continue;
            }
            failedAttempts = 0;
            message = readMessageFromBuffer();
            if (!message.isInvalid()) {
                return message;
            }
        }
        return Message();
    }

  private:
    bool readBytes() {
        char block[kBlockReadSize];
        ssize_t numBytesRead
          = read(inputFD, static_cast<char*>(block), kBlockReadSize);
        if (numBytesRead < 0) {
            if (errno == EAGAIN) {
                return false;
            }
#if EWOULDBLOCK != EAGAIN
            if (errno == EWOULDBLOCK) {
                return false;
            }
#endif
            throw std::system_error(
              errno, std::generic_category(), "PipeReader:readBytes()");
        }
        if (numBytesRead == 0) {
            return false;
        }
        resizeBufferToFit(static_cast<std::size_t>(numBytesRead));
        memcpy(buffer + bufferSize,
               static_cast<char*>(block),
               static_cast<std::size_t>(numBytesRead));
        bufferSize += static_cast<std::size_t>(numBytesRead);
        return true;
    }

    void resizeBufferToFit(std::size_t extraBytes) {
        if (bufferCapacity < bufferSize + extraBytes && bufferReadHead > 0) {
            std::memmove(
              buffer, buffer + bufferReadHead, bufferCapacity - bufferReadHead);
            bufferSize -= bufferReadHead;
            bufferReadHead = 0;
        }
        while (bufferCapacity < bufferSize + extraBytes) {
            auto newBuffer
              = static_cast<std::uint8_t*>(malloc(2 * bufferCapacity));
            memcpy(newBuffer, buffer, bufferSize);
            free(buffer);
            buffer = newBuffer;
            bufferCapacity *= 2;
        }
    }

    Message readMessageFromBuffer() {
        auto message
          = Message::Read(buffer + bufferReadHead, bufferSize - bufferReadHead);
        if (!message.isInvalid()) {
            bufferReadHead += GetMessageSize(message);
        }
        return message;
    }

    int inputFD;
    std::uint8_t* buffer;
    std::size_t bufferReadHead;
    std::size_t bufferSize;
    std::size_t bufferCapacity;
};

class PosixPipeWriter : public PipeWriter {
  public:
    explicit PosixPipeWriter(const int& outputFD): outputFD(outputFD) {
    }

    ~PosixPipeWriter() override {
        ::close(outputFD);
    }

    void sendBytes(const std::uint8_t* bytes, std::size_t numBytes) override {
        std::size_t written = 0;
        while (written < numBytes) {
            auto target = bytes + written;
            std::size_t remaining = numBytes - written;
            ssize_t currentWriteBlockSize = write(outputFD, target, remaining);
            if (currentWriteBlockSize < 0) {
                throw std::system_error(
                  errno, std::generic_category(), "PipeWriter:sendBytes");
            }
            written += static_cast<std::size_t>(currentWriteBlockSize);
        }
    }

    int outputFD;
};

}  // namespace mcga::proc::internal

namespace mcga::proc {

inline std::pair<std::unique_ptr<PipeReader>, std::unique_ptr<PipeWriter>>
  createAnonymousPipe() {
    int fd[2];
    if (pipe(fd) < 0) {
        throw std::system_error(
          errno, std::generic_category(), "createAnonymousPipe:pipe");
    }
    if (fcntl(fd[0], F_SETFL, O_NONBLOCK) < 0) {
        throw std::system_error(
          errno,
          std::generic_category(),
          "createAnonymousPipe:fcntl (set read non-blocking)");
    }
    if (fcntl(fd[1], F_SETFL, O_NONBLOCK) < 0) {
        throw std::system_error(
          errno,
          std::generic_category(),
          "createAnonymousPipe:fcntl (set write non-blocking)");
    }
    return {std::make_unique<internal::PosixPipeReader>(fd[0]),
            std::make_unique<internal::PosixPipeWriter>(fd[1])};
}

inline std::unique_ptr<PipeWriter>
  createLocalClientSocket(const std::string& pathname) {
    sockaddr_un server{};
    server.sun_family = AF_UNIX;
    if (sizeof(server.sun_path) < pathname.length() + 1) {
        throw std::invalid_argument("Cannot connect socket to address: '"
                                    + pathname + "', address too long!");
    }

    int socketFd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (socketFd < 0) {
        throw std::system_error(
          errno, std::generic_category(), "createLocalClientSocket:socket");
    }
    if (fcntl(socketFd, F_SETFL, O_NONBLOCK) < 0) {
        throw std::system_error(
          errno,
          std::generic_category(),
          "createLocalClientSocket:fcntl (set non-blocking)");
    }

    strcpy(static_cast<char*>(server.sun_path), pathname.c_str());

    if (::connect(
          socketFd, reinterpret_cast<sockaddr*>(&server), sizeof(sockaddr_un))
        != 0) {
        ::close(socketFd);
        throw std::system_error(
          errno, std::generic_category(), "createLocalClientSocket:connect");
    }
    return std::make_unique<internal::PosixPipeWriter>(socketFd);
}

inline std::unique_ptr<PipeWriter>
  PipeWriter::OpenFile(const std::string& fileName) {
    int fd = open(fileName.c_str(), O_CREAT | O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        throw std::system_error(errno, std::generic_category(), "open file");
    }
    return std::make_unique<internal::PosixPipeWriter>(fd);
}

}  // namespace mcga::proc
