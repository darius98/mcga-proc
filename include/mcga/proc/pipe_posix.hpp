#pragma once

#include "pipe.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <system_error>

namespace mcga::proc {

class PosixPipeReader : public PipeReader {
  public:
    explicit PosixPipeReader(const int& inputFD)
            : inputFD(inputFD), buffer(static_cast<std::uint8_t*>(malloc(128))),
              bufferReadHead(0), bufferSize(0), bufferCapacity(128) {
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
        char block[128];
        ssize_t numBytesRead = read(inputFD, block, 128);
        if (numBytesRead < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                return false;
            }
            throw std::system_error(
              errno, std::generic_category(), "PipeReader:readBytes()");
        }
        if (numBytesRead == 0) {
            return false;
        }
        resizeBufferToFit((std::size_t)numBytesRead);
        memcpy(buffer + bufferSize, block, static_cast<std::size_t>(numBytesRead));
        bufferSize += numBytesRead;
        return true;
    }

    void resizeBufferToFit(std::size_t extraBytes) {
        if (bufferCapacity < bufferSize + extraBytes && bufferReadHead > 0) {
            memcpy(
              buffer, buffer + bufferReadHead, bufferCapacity - bufferReadHead);
            bufferSize -= bufferReadHead;
            bufferReadHead = 0;
        }
        while (bufferCapacity < bufferSize + extraBytes) {
            auto newBuffer = static_cast<std::uint8_t*>(malloc(2 * bufferCapacity));
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
        close(outputFD);
    }

    void sendBytes(std::uint8_t* bytes, std::size_t numBytes) override {
        std::size_t written = 0;
        while (written < numBytes) {
            auto target = bytes + written;
            std::size_t remaining = numBytes - written;
            ssize_t currentWriteBlockSize = write(outputFD, target, remaining);
            if (currentWriteBlockSize < 0) {
                throw std::system_error(
                  errno, std::generic_category(), "PipeWriter:sendBytes");
            }
            written += currentWriteBlockSize;
        }
    }

    int outputFD;
};

inline std::pair<PipeReader*, PipeWriter*> createAnonymousPipe() {
    int fd[2];
    if (pipe(fd) < 0) {
        throw std::system_error(
          errno, std::generic_category(), "createAnonymousPipe:pipe");
    }
    if (fcntl(fd[0], F_SETFL, O_NONBLOCK) < 0) {
        throw std::system_error(errno,
                                std::generic_category(),
                           "createAnonymousPipe:fcntl (set read non-blocking)");
    }
    if (fcntl(fd[1], F_SETFL, O_NONBLOCK) < 0) {
        throw std::system_error(errno,
                           std::generic_category(),
                           "createAnonymousPipe:fcntl (set write non-blocking");
    }
    return {new PosixPipeReader(fd[0]), new PosixPipeWriter(fd[1])};
}

inline PipeWriter* PipeWriter::OpenFile(const std::string& fileName) {
    int fd = open(fileName.c_str(), O_CREAT | O_WRONLY);
    if (fd < 0) {
        throw std::system_error(errno, std::generic_category(), "open file");
    }
    return new PosixPipeWriter(fd);
}

}  // namespace mcga::proc
