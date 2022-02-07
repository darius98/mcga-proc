#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "serialization.hpp"

namespace mcga::proc {

class Message {
    static constexpr std::size_t prefixSize = alignof(std::max_align_t);

  public:
    template<class... Args>
    static Message Build(const Args&... args) {
        std::size_t numBytes = 0;
        write_from([&numBytes](const void*, std::size_t size) {
            numBytes += size;
        }, args...);

        std::uint8_t* payload = Allocate(numBytes + prefixSize);
        std::memset((void*)payload, 0, prefixSize);
        copy_data(payload, &numBytes, sizeof(numBytes));

        std::size_t cursor = prefixSize;
        write_from([payload, &cursor](const void* data, std::size_t size) {
            copy_data(payload + cursor, data, size);
            cursor += size;
        }, args...);
        return Message(payload);
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
        copy_data(messagePayload, src, expectedSize);
        return Message(messagePayload);
    }

    Message() = default;

    Message(const Message& other) {
        if (!other.isInvalid()) {
            auto size = other.getSize();
            payload.reset(Allocate(size));
            copy_data(payload.get(), other.payload.get(), size);
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
            copy_data(payload.get(), other.payload.get(), size);
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
        read_into([this](void* buf, std::size_t size) {
            copy_data(buf, at(readHead), size);
            readHead += size;
        }, obj);
        return *this;
    }

    template<class T>
    T read() {
        T obj;
        read_into([this](void* buf, std::size_t size) {
            copy_data(buf, at(readHead), size);
            readHead += size;
        }, obj);
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
    template<class In, class Out>
    static void copy_data(Out* out, const In* in, std::size_t size) {
        std::memcpy((void*)out, (const void*)in, size);
    }

    explicit Message(uint8_t* payload) noexcept: payload(payload) {
    }

    std::size_t getSize() const {
        return prefixSize + ExpectedContentSizeFromBuffer(payload.get());
    }

    std::uint8_t* at(std::size_t pos) const {
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

    friend class PipeReader;
    friend class PipeWriter;
};

}  // namespace mcga::proc
