#pragma once

#include <string>

namespace mcga::proc {

class Message {
  public:
    static const Message INVALID;

    template<class... Args>
    static Message Build(const Args... args) {
        BytesCounter counter;
        counter.add(args...);
        Builder messageBuilder(counter.getNumBytesConsumed());
        messageBuilder.add(args...);
        return messageBuilder.build();
    }

    static Message Read(const void* src, std::size_t maxSize);

    Message();
    Message(const Message& other);
    Message(Message&& other) noexcept;

    ~Message();

    Message& operator=(const Message& other);
    Message& operator=(Message&& other) noexcept;

    bool operator==(const Message& other) const;

    bool isInvalid() const;

    template<class T>
    Message& operator>>(T& obj) {
        obj = *reinterpret_cast<T*>(payload + readHead);
        readHead += sizeof(obj);
        return *this;
    }

    template<class T>
    T read() {
        T obj;
        (*this) >> obj;
        return obj;
    }

  private:
    explicit Message(uint8_t* payload) noexcept;

    std::size_t getSize() const;

    void copyContent(const Message& other);

    std::size_t readHead = sizeof(std::size_t);
    std::uint8_t* payload;

    // helper internal classes

    static std::size_t ExpectedContentSizeFromBuffer(const void* buffer);

    static std::uint8_t* Allocate(std::size_t numBytes);

    class BytesConsumer {
      public:
        virtual BytesConsumer& addBytes(const void* bytes, std::size_t numBytes)
          = 0;

        template<class T>
        BytesConsumer& add(const T& obj) {
            return addBytes(&obj, sizeof(obj));
        }

        template<class T1, class T2, class... Args>
        BytesConsumer& add(const T1& a1, const T2& a2, const Args... args) {
            add(a1);
            return add(a2, args...);
        }

        template<class T>
        BytesConsumer& operator<<(const T& obj) {
            return add(obj);
        }
    };

    class BytesCounter : public BytesConsumer {
      public:
        BytesCounter& addBytes(const void* bytes,
                               std::size_t numBytes) override;

        std::size_t getNumBytesConsumed() const;

      private:
        std::size_t bytesConsumed = 0;
    };

    class Builder : public BytesConsumer {
      public:
        explicit Builder(std::size_t size);

        Builder& addBytes(const void* bytes, std::size_t numBytes) override;

        Message build();

      private:
        std::uint8_t* payloadBuilder;
        std::size_t cursor;
    };

    friend class PipeReader;
    friend class PipeWriter;
};

template<>
Message& Message::operator>>(std::string& obj);

template<>
Message::BytesConsumer& Message::BytesConsumer::add(const std::string& obj);

template<>
Message::BytesConsumer& Message::BytesConsumer::add(const Message& obj);

}  // namespace mcga::proc
