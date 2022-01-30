#pragma once

#include <cstdlib>

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace mcga::proc {

class Message {
    static constexpr std::size_t prefixSize = alignof(std::max_align_t);

  public:
    template<class... Args>
    static Message Build(const Args&... args) {
        BytesCounter counter;
        counter.add(args...);
        Builder messageBuilder(counter.getNumBytesConsumed());
        messageBuilder.add(args...);
        return messageBuilder.build();
    }

    static Message Read(const void* src, std::size_t maxSize) {
        if (maxSize < prefixSize) {
            return Message();
        }
        auto expectedSize = ExpectedContentSizeFromBuffer(src) + prefixSize;
        if (maxSize < expectedSize) {
            return Message();
        }
        auto messagePayload = Allocate(expectedSize);
        std::memcpy(messagePayload, src, expectedSize);
        return Message(messagePayload);
    }

    Message() = default;

    Message(const Message& other) {
        if (!other.isInvalid()) {
            auto size = other.getSize();
            payload.reset(Allocate(size));
            std::memcpy(payload.get(), other.payload.get(), size);
        }
    }

    Message(Message&& other) noexcept: payload(std::move(other.payload)) {
    }

    Message& operator=(const Message& other) {
        if (this == &other) {
            return *this;
        }
        readHead = prefixSize;
        payload.reset();
        if (!other.isInvalid()) {
            auto size = other.getSize();
            payload.reset(Allocate(size));
            std::memcpy(payload.get(), other.payload.get(), size);
        }
        return *this;
    }

    Message& operator=(Message&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        readHead = prefixSize;
        payload = std::move(other.payload);
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
        static_assert(
          std::is_trivially_copyable_v<T>,
          "Unable to automatically deserialize type. Please specialize "
          "mcga::proc::Message::operator>>() for this type.");

        obj.~T();
        std::memcpy(&obj, at(readHead), sizeof(T));
        // TODO: This doesn't feel right (we don't start object lifetime for
        //  obj after memcpy). Use something like std::launder?
        readHead += sizeof(T);
        return *this;
    }

    template<class T>
    Message& operator>>(std::optional<T>& obj) {
        const auto hasValue = read<bool>();
        if (hasValue) {
            obj = read<T>();
        } else {
            obj = std::nullopt;
        }
        return *this;
    }

    template<class T>
    Message& operator>>(std::vector<T>& obj) {
        const auto size = read<typename std::vector<T>::size_type>();
        obj.resize(size);
        for (auto& entry: obj) {
            (*this) >> entry;
        }
        return *this;
    }

    Message& operator>>(std::string& obj) {
        const auto size = read<std::string::size_type>();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        obj.assign(
          reinterpret_cast<char*>(at(readHead)),
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<char*>(at(readHead + size)));
        readHead += size;
        return *this;
    }

    template<class T>
    T read() {
        T obj;
        (*this) >> obj;
        return obj;
    }

    [[nodiscard]] std::string debugPayloadAsInts() const {
        const auto size = getSize();
        std::string hex;
        hex.reserve((size - prefixSize) * 4);
        for (std::size_t i = prefixSize; i < size; i++) {
            hex += std::to_string((int)*at(i));
            hex += ' ';
        }
        return hex;
    }

    [[nodiscard]] std::string debugPayloadAsHex() const {
        const char hexDigits[] = "0123456789ABCDEF";
        const auto size = getSize() - prefixSize;
        std::string hex;
        hex.reserve(size * 2);
        for (std::size_t i = 0; i < size; i++) {
            hex += hexDigits[payload[prefixSize + i] / 16];
            hex += hexDigits[payload[prefixSize + i] % 16];
        }
        return hex;
    }

    [[nodiscard]] std::string debugPayloadAsChars() const {
        return {(char*)payload.get() + prefixSize,
                (char*)payload.get() + getSize()};
    }

  private:
    explicit Message(uint8_t* payload) noexcept: payload(payload) {
    }

    std::size_t getSize() const {
        return prefixSize + ExpectedContentSizeFromBuffer(payload.get());
    }

    std::uint8_t* at(std::size_t pos) const {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return &payload[pos];
    }

    std::size_t readHead = prefixSize;
    std::unique_ptr<std::uint8_t[]> payload;

    // helper internal classes
    static std::size_t ExpectedContentSizeFromBuffer(const void* buffer) {
        return *static_cast<const std::size_t*>(buffer);
    }

    static std::uint8_t* Allocate(std::size_t numBytes) {
        return new std::uint8_t[numBytes];
    }

    class BytesConsumer {
      public:
        virtual BytesConsumer& addBytes(const void* bytes, std::size_t numBytes)
          = 0;

        template<class T>
        BytesConsumer& add(const T& obj) {
            static_assert(
              std::is_trivially_copyable_v<T>,
              "Unable to automatically serialize type. Please specialize "
              "mcga::proc::Message::BytesConsumer::add() for this type.");
            return addBytes(&obj, sizeof(obj));
        }

        template<class T>
        BytesConsumer& add(const std::optional<T>& obj) {
            add(obj.has_value());
            if (obj.has_value()) {
                add(*obj);
            }
            return *this;
        }

        template<class T>
        BytesConsumer& add(const std::vector<T>& obj) {
            add(obj.size());
            for (const auto& entry: obj) {
                add(entry);
            }
            return *this;
        }

        BytesConsumer& add(const std::string& obj) {
            add(obj.size());
            addBytes(obj.c_str(), obj.size());
            return *this;
        }

        BytesConsumer& add(const Message& obj) {
            auto contentSize = ExpectedContentSizeFromBuffer(obj.payload.get());
            addBytes(obj.at(prefixSize), contentSize);
            return *this;
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
        BytesCounter& addBytes(const void* /*bytes*/,
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
                : payloadBuilder(Allocate(size + prefixSize)) {
            memset(payloadBuilder, 0, prefixSize);
            std::memcpy(payloadBuilder, &size, sizeof(std::size_t));
            cursor = prefixSize;
        }

        Builder& addBytes(const void* bytes, std::size_t numBytes) override {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::memcpy(payloadBuilder + cursor, bytes, numBytes);
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

}  // namespace mcga::proc
