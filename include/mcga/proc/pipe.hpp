#pragma once

#include <string>
#include <vector>

#include "message.hpp"

namespace mcga::proc {

class PipeReader {
  public:
    virtual ~PipeReader() = default;

    // If maxConsecutiveFailedReadAttempts = -1, we block instead.
    virtual Message getNextMessage(int maxConsecutiveFailedReadAttempts) = 0;

    Message getNextMessage() {
        return getNextMessage(-1);
    }

  protected:
    static std::size_t GetMessageSize(const Message& message) {
        return message.getSize();
    }
};

class PipeWriter {
  public:
    static PipeWriter* OpenFile(const std::string& fileName);

    virtual ~PipeWriter() = default;

    void sendMessage(const Message& message) {
        sendBytes(message.payload, message.getSize());
    }

    template<class... Args>
    void sendMessage(const Args... args) {
        sendMessage(Message::Build(args...));
    }

  private:
    virtual void sendBytes(std::uint8_t* bytes, std::size_t numBytes) = 0;
};

std::pair<PipeReader*, PipeWriter*> createAnonymousPipe();

}  // namespace mcga::proc

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#include "pipe_posix.hpp"
#else
#error "Non-unix systems are not currently supported by mcga::proc."
#endif
