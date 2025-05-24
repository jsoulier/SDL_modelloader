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

    Serializer(void* data, uint32_t size)
        : is_writer(false)
        , reader{static_cast<uint8_t*>(data), size}
        , writer{}
        , offset{0} {}

    template<typename T>
    void visit(T& data)
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

    const std::vector<uint8_t>& get_writer() const
    {
        return writer;
    }

private:
    bool is_writer;

    std::span<uint8_t> reader;
    std::vector<uint8_t> writer;

    uint32_t offset;
};