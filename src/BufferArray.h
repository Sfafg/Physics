#pragma once
#include <VG/VG.h>
#include <memory>
#include <vector>

class BufferArray
{
public:
    BufferArray() {}

    BufferArray(uint64_t byteSize, vg::Flags<vg::BufferUsage> usage, vg::Flags<vg::MemoryProperty> memoryProperties)
        :buffer(new vg::Buffer(byteSize, usage)), memoryProperties(memoryProperties), usage(usage)
    {
        vg::Allocate(buffer.get(), memoryProperties);
    }

    template <typename T>
    BufferArray(const std::vector<T>& data, vg::Flags<vg::BufferUsage> usage, vg::Flags<vg::MemoryProperty> memoryProperties, const vg::Queue* transferQueue = nullptr)
        : BufferArray(data.size() * sizeof(T), usage, memoryProperties)
    {
        Write(data, 0, transferQueue);
    }

    template <typename T>
    void Write(const std::vector<T>& data, uint32_t offset = 0, const vg::Queue* transferQueue = nullptr)
    {
        assert((data.size() + offset) * sizeof(T) <= buffer->GetSize());

        if (memoryProperties.IsSet(vg::MemoryProperty::HostVisible))
        {
            memcpy((T*) buffer->MapMemory() + offset, data.data(), data.size() * sizeof(T));
            buffer->UnmapMemory();
        }
        else if (memoryProperties.IsSet(vg::MemoryProperty::DeviceLocal))
        {
            vg::Buffer stagingBuffer(data.size() * sizeof(T), { vg::BufferUsage::TransferSrc });
            vg::Allocate(&stagingBuffer, { vg::MemoryProperty::HostVisible });
            memcpy(stagingBuffer.MapMemory(), data.data(), data.size() * sizeof(T));
            buffer->UnmapMemory();

            vg::CmdBuffer(*transferQueue)
                .Begin()
                .Append(vg::cmd::CopyBuffer(stagingBuffer, *buffer, { vg::BufferCopyRegion(data.size() * sizeof(T),0,offset * sizeof(T)) }))
                .End()
                .Submit()
                .Await();
        }
    }

    template <typename T>
    std::vector<T> Read(uint32_t size, uint32_t offset = 0, const vg::Queue* transferQueue = nullptr)
    {
        assert((size + offset) * sizeof(T) <= buffer->GetSize());

        std::vector<T> data(size);
        if (memoryProperties.IsSet(vg::MemoryProperty::HostVisible))
        {
            memcpy(data.data(), (T*) buffer->MapMemory() + offset, data.size() * sizeof(T));
            buffer->UnmapMemory();
        }
        else if (memoryProperties.IsSet(vg::MemoryProperty::DeviceLocal))
        {
            vg::Buffer stagingBuffer(data.size() * sizeof(T), { vg::BufferUsage::TransferDst });
            vg::Allocate(&stagingBuffer, { vg::MemoryProperty::HostVisible });
            vg::CmdBuffer(*transferQueue)
                .Begin()
                .Append(vg::cmd::CopyBuffer(*buffer, stagingBuffer, { vg::BufferCopyRegion(data.size() * sizeof(T),offset * sizeof(T),0) }))
                .End()
                .Submit()
                .Await();

            memcpy(data.data(), stagingBuffer.MapMemory(), data.size() * sizeof(T));
            buffer->UnmapMemory();
        }
        return data;
    }

    template <typename T>
    void Resize(int count, const vg::Queue* transferQueue)
    {
        BufferArray newBuffer(sizeof(T) * count, usage, memoryProperties);

        uint32_t minSize = newBuffer.buffer->GetSize();
        if (buffer->GetSize() < minSize)
            minSize = buffer->GetSize();

        vg::CmdBuffer(*transferQueue)
            .Begin()
            .Append(vg::cmd::CopyBuffer(*buffer, newBuffer, { vg::BufferCopyRegion(minSize,0,0) }))
            .End()
            .Submit()
            .Await();

        *this = std::move(newBuffer);
    }

    operator vg::Buffer& () const
    {
        return *buffer;
    }

    operator vg::BufferHandle() const
    {
        return *buffer;
    }

    // private:
    std::shared_ptr<vg::Buffer> buffer;
    vg::Flags<vg::BufferUsage> usage;
    vg::Flags<vg::MemoryProperty> memoryProperties;
};