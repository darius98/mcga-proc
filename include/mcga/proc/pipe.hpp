#pragma once

#include <string>
#include <vector>

#include <mcga/proc/message.hpp>

namespace mcga::proc {

class PipeReader {
  public:
    virtual ~PipeReader() = default;

    // If maxConsecutiveFailedReadAttempts = -1, we block instead.
    virtual Message getNextMessage(int maxConsecutiveFailedReadAttempts) = 0;

    Message getNextMessage();

  protected:
    static std::size_t GetMessageSize(const Message& message);
};

class PipeWriter {
  public:
    static PipeWriter* OpenFile(const std::string& fileName);

    virtual ~PipeWriter() = default;

    void sendMessage(const Message& message);

    template<class... Args>
    void sendMessage(const Args... args) {
        sendMessage(Message::Build(args...));
    }

  private:
    virtual void sendBytes(std::uint8_t* bytes, std::size_t numBytes) = 0;
};

std::pair<PipeReader*, PipeWriter*> createAnonymousPipe();

}  // namespace mcga::proc
