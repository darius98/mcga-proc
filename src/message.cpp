#include <mcga/proc/message.hpp>

#include <cstdlib>
#include <cstring>

using namespace std;

namespace mcga::proc {

const Message Message::INVALID = Message(nullptr);

Message Message::Read(const void* src, size_t maxSize) {
    if (maxSize < sizeof(size_t)) {
        return INVALID;
    }
    auto expectedSize = ExpectedContentSizeFromBuffer(src) + sizeof(size_t);
    if (expectedSize > maxSize) {
        return INVALID;
    }
    auto messagePayload = Allocate(expectedSize);
    memcpy(messagePayload, src, expectedSize);
    return Message(messagePayload);
}

Message::Message(): Message(INVALID) {
}

Message::Message(const Message& other) {
    copyContent(other);
}

Message::Message(Message&& other) noexcept: payload(other.payload) {
    if (this != &other) {
        other.payload = nullptr;
    }
}

Message::Message(uint8_t* payload) noexcept: payload(payload) {
}

Message::~Message() {
    if (payload != nullptr) {
        free(payload);
    }
}

Message& Message::operator=(const Message& other) {
    if (this == &other) {
        return *this;
    }
    if (payload != nullptr) {
        free(payload);
    }
    copyContent(other);
    readHead = sizeof(size_t);
    return *this;
}

Message& Message::operator=(Message&& other) noexcept {
    readHead = sizeof(size_t);
    payload = other.payload;
    if (this != &other) {
        other.payload = nullptr;
    }
    return *this;
}

void Message::copyContent(const Message& other) {
    if (other.isInvalid()) {
        payload = nullptr;
    } else {
        auto size = other.getSize();
        payload = Allocate(size);
        memcpy(payload, other.payload, size);
    }
}

size_t Message::getSize() const {
    return sizeof(size_t) + ExpectedContentSizeFromBuffer(payload);
}

bool Message::operator==(const Message& other) const {
    return this == &other || (isInvalid() && other.isInvalid());
}

bool Message::isInvalid() const {
    return payload == nullptr;
}

template<>
Message& Message::operator>>(string& obj) {
    decltype(obj.size()) size;
    (*this) >> size;
    obj.assign(reinterpret_cast<char*>(payload) + readHead,
               reinterpret_cast<char*>(payload) + readHead + size);
    readHead += size;
    return *this;
}

size_t Message::ExpectedContentSizeFromBuffer(const void* buffer) {
    return *static_cast<const size_t*>(buffer);
}

uint8_t* Message::Allocate(size_t numBytes) {
    return static_cast<uint8_t*>(malloc(numBytes));
}

template<>
Message::BytesConsumer& Message::BytesConsumer::add(const string& obj) {
    add(obj.size());
    addBytes(obj.c_str(), obj.size());
    return *this;
}

template<>
Message::BytesConsumer& Message::BytesConsumer::add(const Message& obj) {
    auto contentSize = ExpectedContentSizeFromBuffer(obj.payload);
    addBytes(obj.payload + sizeof(size_t), contentSize);
    return *this;
}

Message::BytesCounter& Message::BytesCounter::addBytes(const void*,
                                                       size_t numBytes) {
    bytesConsumed += numBytes;
    return *this;
}

size_t Message::BytesCounter::getNumBytesConsumed() const {
    return bytesConsumed;
}

Message::Builder::Builder(size_t size)
        : payloadBuilder(Allocate(size + sizeof(size_t))) {
    memcpy(payloadBuilder, &size, sizeof(size_t));
    cursor = sizeof(size_t);
}

Message::Builder& Message::Builder::addBytes(const void* bytes,
                                             size_t numBytes) {
    memcpy(payloadBuilder + cursor, bytes, numBytes);
    cursor += numBytes;
    return *this;
}

Message Message::Builder::build() {
    return Message(payloadBuilder);
}

}  // namespace mcga::proc
