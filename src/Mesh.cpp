#include "Mesh.h"

Mesh::Mesh() : materialID(0) {}

Mesh::Mesh(Span<const uint64_t> attributesSize, uint32_t materialID)
    : materialID(materialID)
{
    for (auto&& size : attributesSize)
        attributes.push_back(vg::Buffer(size, { vg::BufferUsage::TransferSrc, vg::BufferUsage::TransferDst,vg::BufferUsage::VertexBuffer }));

    if (attributes.size() != 0)
        vg::Allocate(attributes, { vg::MemoryProperty::DeviceLocal });
}

Mesh::Mesh(vg::IndexType indexType, uint32_t triangleCount, Span<const uint64_t> attributesSize, uint32_t materialID)
    :indexType(indexType), materialID(materialID)
{
    uint64_t triangleBufferSize = triangleCount;
    switch (indexType)
    {
    case vg::IndexType::Uint8:break;
    case vg::IndexType::Uint16:triangleBufferSize *= 2; break;
    case vg::IndexType::Uint32:triangleBufferSize *= 4; break;
    default:break;
    }
    triangleBuffer = vg::Buffer(triangleBufferSize, { vg::BufferUsage::TransferSrc, vg::BufferUsage::TransferDst,vg::BufferUsage::IndexBuffer });
    vg::Allocate(triangleBuffer, { vg::MemoryProperty::DeviceLocal });

    for (auto&& size : attributesSize)
        attributes.push_back(vg::Buffer(size, { vg::BufferUsage::TransferSrc, vg::BufferUsage::TransferDst,vg::BufferUsage::VertexBuffer }));

    if (attributes.size() != 0)
        vg::Allocate(attributes, { vg::MemoryProperty::DeviceLocal });
}


void Mesh::WriteTriangles(void* data, uint64_t size, uint64_t offset)
{
    vg::Buffer stagingBuffer(size, { vg::BufferUsage::TransferSrc });
    vg::Allocate(stagingBuffer, { vg::MemoryProperty::HostVisible });
    memcpy(stagingBuffer.MapMemory(), data, size);

    vg::CmdBuffer(vg::currentDevice->GetQueue(0)).Begin().Append(
        vg::cmd::CopyBuffer(stagingBuffer, triangleBuffer, { vg::BufferCopyRegion(size,0,offset) })
    ).End().Submit().Await();
}

void Mesh::ReadTriangles(void*& data, uint64_t size, uint64_t offset)
{
    size = std::min(size, triangleBuffer.GetSize() - offset);

    vg::Buffer stagingBuffer(size, { vg::BufferUsage::TransferDst });
    vg::Allocate(stagingBuffer, { vg::MemoryProperty::HostVisible, vg::MemoryProperty::HostCoherent });
    vg::CmdBuffer(vg::currentDevice->GetQueue(0)).Begin().Append(
        vg::cmd::CopyBuffer(triangleBuffer, stagingBuffer, { vg::BufferCopyRegion(size,offset,0) })
    ).End().Submit().Await();

    data = malloc(size);
    memcpy(data, stagingBuffer.MapMemory(), size);
}

void Mesh::WriteAttribute(uint32_t attributeIndex, void* data, uint64_t size, uint64_t offset)
{
    vg::Buffer stagingBuffer(size, { vg::BufferUsage::TransferSrc });
    vg::Allocate(stagingBuffer, { vg::MemoryProperty::HostVisible });
    memcpy(stagingBuffer.MapMemory(), data, size);

    vg::CmdBuffer(vg::currentDevice->GetQueue(0)).Begin().Append(
        vg::cmd::CopyBuffer(stagingBuffer, attributes[attributeIndex], { vg::BufferCopyRegion(size,0,offset) })
    ).End().Submit().Await();
}

void Mesh::ReadAttribute(uint32_t attributeIndex, void*& data, uint64_t size, uint64_t offset)
{
    size = std::min(size, attributes[attributeIndex].GetSize() - offset);

    vg::Buffer stagingBuffer(size, { vg::BufferUsage::TransferDst });
    vg::Allocate(stagingBuffer, { vg::MemoryProperty::HostVisible, vg::MemoryProperty::HostCoherent });
    vg::CmdBuffer(vg::currentDevice->GetQueue(0)).Begin().Append(
        vg::cmd::CopyBuffer(attributes[attributeIndex], stagingBuffer, { vg::BufferCopyRegion(size,offset,0) })
    ).End().Submit().Await();

    data = malloc(size);
    memcpy(data, stagingBuffer.MapMemory(), size);
}