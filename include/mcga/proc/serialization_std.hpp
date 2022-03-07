#pragma once

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "serialization.hpp"

namespace mcga::proc {

template<class T>
void read_custom(binary_reader auto& reader, std::optional<T>& obj) {
    bool hasValue;
    read_into(reader, hasValue);
    if (hasValue) {
        T value;
        read_into(reader, value);
        obj = std::move(value);
    } else {
        obj = std::nullopt;
    }
}

template<class T>
void write_custom(binary_writer auto& writer, const std::optional<T>& obj) {
    write_from(writer, obj.has_value());
    if (obj.has_value()) {
        write_from(writer, obj.value());
    }
}

template<class T>
void read_custom(binary_reader auto& reader, std::vector<T>& obj) {
    typename std::vector<T>::size_type size;
    read_into(reader, size);
    obj.resize(size);
    for (auto& entry: obj) {
        read_into(reader, entry);
    }
}

template<class T>
void write_custom(binary_writer auto& writer, const std::vector<T>& obj) {
    write_from(writer, obj.size());
    for (const auto& entry: obj) {
        write_from(writer, entry);
    }
}

void read_custom(binary_reader auto& reader, std::string& obj) {
    typename std::string::size_type size;
    read_into(reader, size);
    obj.resize(size);
    reader(obj.data(), obj.size());
}

void write_custom(binary_writer auto& writer, const std::string& obj) {
    write_from(writer, obj.size());
    writer(obj.c_str(), obj.size());
}

}  // namespace mcga::proc
