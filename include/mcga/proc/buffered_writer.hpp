#pragma once

#include "serialization.hpp"

namespace mcga::proc {

template<std::size_t BufferSize, binary_writer Writer>
class BufferedWriter {
    Writer writer;
    char buffer[BufferSize]{};
    char* cursor = buffer;
    char* bufferEnd = buffer + BufferSize;

  public:
    explicit BufferedWriter(Writer writer): writer(std::move(writer)) {
    }

    void operator()(const void* raw_data, std::size_t size) {
        if (size > BufferSize) {
            flush();
            writer(raw_data, size);
            return;
        }
        if (cursor + size <= bufferEnd) {
            std::memcpy(cursor, raw_data, size);
            cursor += size;
        } else {
            const auto bufferRemaining = bufferEnd - cursor;
            std::memcpy(cursor, raw_data, bufferRemaining);
            writer(buffer, BufferSize);
            std::memcpy(buffer, (char*)raw_data + bufferRemaining, size - bufferRemaining);
            cursor = buffer + size - bufferRemaining;
        }
    }

    void flush() {
        if (cursor != buffer) {
            writer(buffer, cursor - buffer);
            cursor = buffer;
        }
    }
};

template<binary_writer Writer, std::size_t BufferSize=256>
BufferedWriter(Writer) -> BufferedWriter<BufferSize, Writer>;

}  // namespace mcga::test
