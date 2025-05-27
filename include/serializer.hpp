#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

class Serializer
{
public:
    Serializer()
        : is_writer(true)
        , reader{}
        , writer{}
        , offset{0} {}

    Serializer(const void* data, uint32_t size)
        : is_writer(false)
        , reader{static_cast<uint8_t*>(const_cast<void*>(data)), size}
        , writer{}
        , offset{0} {}

    template<typename T>
    void serialize(T& data)
    {
        if (is_writer)
        {
            uint32_t offset = writer.size();
            writer.resize(offset + sizeof(T));
            std::memcpy(writer.data() + offset, &data, sizeof(T));
        }
        else
        {
            assert(offset + sizeof(T) <= reader.size());
            std::memcpy(&data, reader.data() + offset, sizeof(T));
            offset += sizeof(T);
        }
    }

    const void* get_data() const
    {
        return static_cast<const void*>(writer.data());
    }

    uint32_t get_size() const
    {
        return static_cast<uint32_t>(writer.size());
    }

private:
    bool is_writer;

    std::span<uint8_t> reader;
    std::vector<uint8_t> writer;

    uint32_t offset;
};