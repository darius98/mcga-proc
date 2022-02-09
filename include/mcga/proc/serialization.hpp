#pragma once

#include <cstdint>

namespace mcga::proc {

template<class T>
concept binary_reader = requires(T& t, void* dst, std::size_t size) {
    {t(dst, size)};
};

template<class T>
void read_into(binary_reader auto&& reader, T& obj) {
    if constexpr (requires { {obj.mcga_read(reader)}; }) {
        obj.mcga_read(reader);
    } else if constexpr (requires { {mcga_read(reader, obj)}; }) {
        mcga_read(reader, obj);
    } else {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Unable to automatically deserialize type. Please  "
                      "add an mcga_read(binary_reader auto& reader) or "
                      "specialize mcga_read(binary_reader auto& reader, T& "
                      "obj) for it.");
        static_assert(!std::is_pointer_v<T>,
                      "Unable to automatically deserialize raw pointer.");
        obj.~T();
        reader(&obj, sizeof(T));
        // TODO: This doesn't feel right (we don't start object lifetime for
        //  obj after memcpy). Use something like std::bless?
    }
}

template<class T, class... Args>
void read_into(binary_reader auto&& reader, T& obj, Args&... args) {
    read_into(reader, obj);
    (read_into(reader, args), ...);
}

template<class T>
T read_as(binary_reader auto&& reader) {
    T obj;
    read_into(reader, obj);
    return obj;
}

template<class T>
concept binary_writer = requires(T& t, const void* source, std::size_t size) {
    {t(source, size)};
};

template<class T>
void write_from(binary_writer auto&& writer, const T& obj) {
    if constexpr (requires { {obj.mcga_write(writer)}; }) {
        obj.mcga_write(writer);
    } else if constexpr (requires { {mcga_write(writer, obj)}; }) {
        mcga_write(writer, obj);
    } else {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Unable to automatically serialize type. Please  "
                      "add an mcga_write(binary_writer auto& writer) method to "
                      "this type, or specialize "
                      "mcga_write(binary_writer auto& writer, T& obj) for it.");
        static_assert(!std::is_pointer_v<T>,
                      "Unable to automatically serialize raw pointer.");
        obj.~T();
        writer(&obj, sizeof(T));
        // TODO: This doesn't feel right (we don't start object lifetime for
        //  obj after memcpy). Use something like std::launder?}
    }
}

template<class T, class... Args>
void write_from(binary_writer auto&& writer,
                const T& obj,
                const Args&... args) {
    write_from(writer, obj);
    (write_from(writer, args), ...);
}

}  // namespace mcga::proc
