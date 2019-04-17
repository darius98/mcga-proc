#pragma once

#include <string>

namespace mcga::proc {

class Message {
  public:
    template<class... Args>
    static Message Build(const Args... args) {
        BytesCounter counter;
        counter.add(args...);
        Builder messageBuilder(counter.getNumBytesConsumed());
        messageBuilder.add(args...);
        return messageBuilder.build();
    }

    static Message Read(const void* src, std::size_t maxSize) {
        if (maxSize < sizeof(size_t)) {
            return Message();
        }
        auto expectedSize = ExpectedContentSizeFromBuffer(src) + sizeof(size_t);
        if (expectedSize > maxSize) {
            return Message();
        }
        auto messagePayload = Allocate(expectedSize);
        memcpy(messagePayload, src, expectedSize);
        return Message(messagePayload);
    }

    Message(): payload(nullptr) {
    }

    Message(const Message& other) {
        copyContent(other);
    }

    Message(Message&& other) noexcept: payload(other.payload) {
        if (this != &other) {
            other.payload = nullptr;
        }
    }

    ~Message() {
        if (payload != nullptr) {
            free(payload);
        }
    }

    Message& operator=(const Message& other) {
        if (this == &other) {
            return *this;
        }
        if (payload != nullptr) {
            free(payload);
        }
        copyContent(other);
        readHead = sizeof(std::size_t);
        return *this;
    }

    Message& operator=(Message&& other) noexcept {
        readHead = sizeof(std::size_t);
        payload = other.payload;
        if (this != &other) {
            other.payload = nullptr;
        }
        return *this;
    }

    bool operator==(const Message& other) const {
        return this == &other || (isInvalid() && other.isInvalid());
    }

    bool isInvalid() const {
        return payload == nullptr;
    }

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
    explicit Message(uint8_t* payload) noexcept: payload(payload) {
    }

    std::size_t getSize() const {
        return sizeof(std::size_t) + ExpectedContentSizeFromBuffer(payload);
    }

    void copyContent(const Message& other) {
        if (other.isInvalid()) {
            payload = nullptr;
        } else {
            auto size = other.getSize();
            payload = Allocate(size);
            memcpy(payload, other.payload, size);
        }
    }

    std::size_t readHead = sizeof(std::size_t);
    std::uint8_t* payload;

    // helper internal classes
    static std::size_t ExpectedContentSizeFromBuffer(const void* buffer) {
        return *static_cast<const std::size_t*>(buffer);
    }

    static std::uint8_t* Allocate(std::size_t numBytes) {
        return static_cast<std::uint8_t*>(malloc(numBytes));
    }

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
                               std::size_t numBytes) override {
            bytesConsumed += numBytes;
            return *this;
        }

        std::size_t getNumBytesConsumed() const {
            return bytesConsumed;
        }

      private:
        std::size_t bytesConsumed = 0;
    };

    class Builder : public BytesConsumer {
      public:
        explicit Builder(std::size_t size)
                : payloadBuilder(Allocate(size + sizeof(std::size_t))) {
            memcpy(payloadBuilder, &size, sizeof(std::size_t));
            cursor = sizeof(std::size_t);
        }

        Builder& addBytes(const void* bytes, std::size_t numBytes) override {
            memcpy(payloadBuilder + cursor, bytes, numBytes);
            cursor += numBytes;
            return *this;
        }

        Message build() {
            return Message(payloadBuilder);
        }

      private:
        std::uint8_t* payloadBuilder;
        std::size_t cursor;
    };

    friend class PipeReader;
    friend class PipeWriter;
};

template<>
inline Message& Message::operator>>(std::string& obj) {
    decltype(obj.size()) size;
    (*this) >> size;
    obj.assign(reinterpret_cast<char*>(payload) + readHead,
               reinterpret_cast<char*>(payload) + readHead + size);
    readHead += size;
    return *this;
}

template<>
inline Message::BytesConsumer&
  Message::BytesConsumer::add(const std::string& obj) {
    add(obj.size());
    addBytes(obj.c_str(), obj.size());
    return *this;
}

template<>
inline Message::BytesConsumer& Message::BytesConsumer::add(const Message& obj) {
    auto contentSize = ExpectedContentSizeFromBuffer(obj.payload);
    addBytes(obj.payload + sizeof(std::size_t), contentSize);
    return *this;
}

}  // namespace mcga::proc
