#pragma once

#include <cstring>

#include "serialization.hpp"

namespace mcga::proc {

template<std::size_t BufferSize, binary_writer Writer>
class BufferedWriter {
    Writer writer;
    char buffer[BufferSize]{};
    char* cursor = buffer;

  public:
    explicit BufferedWriter(Writer writer): writer(std::move(writer)) {
    }

    void operator()(const void* raw_data, std::size_t size) {
        if (size > BufferSize) {
            flush();
            writer(raw_data, size);
            return;
        }
        const auto remSize = BufferSize - (cursor - buffer);
        if (size <= remSize) {
            std::memcpy(cursor, raw_data, size);
            cursor += size;
        } else {
            std::memcpy(cursor, raw_data, remSize);
            writer(buffer, BufferSize);
            std::memcpy(buffer,
                        static_cast<const char*>(raw_data) + remSize,
                        size - remSize);
            cursor = buffer + size - remSize;
        }
    }

    void flush() {
        if (cursor != buffer) {
            writer(buffer, cursor - buffer);
            cursor = buffer;
        }
    }
};

template<binary_writer Writer, std::size_t BufferSize = 256>
BufferedWriter(Writer) -> BufferedWriter<BufferSize, Writer>;

}  // namespace mcga::proc
