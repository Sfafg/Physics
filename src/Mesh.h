#pragma once
#include <memory>
#include "VG/VG.h"
#include "BufferArray.h"

struct Mesh
{
    std::vector<BufferArray> attributes;
    BufferArray indexData;
    vg::IndexType indexType;
    union
    {
        uint32_t vertexCount;
        uint32_t indexCount;
    };
    uint32_t materialID;

    Mesh() : attributes(), vertexCount(0), materialID(-1) {}

    Mesh(const std::vector<BufferArray>& attributes, uint32_t vertexCount, uint32_t materialID, const BufferArray& uniformData = {})
        : attributes(attributes), indexType(vg::IndexType::Uint32), vertexCount(vertexCount), materialID(materialID)
    {}

    Mesh(const std::vector<BufferArray>& attributes, BufferArray indexData, vg::IndexType indexType, uint32_t indexCount, uint32_t materialID, const BufferArray& uniformData = {})
        : attributes(attributes), indexData(indexData), indexType(indexType), indexCount(indexCount), materialID(materialID)
    {}

    BufferArray& GetAttribute(uint32_t index)
    {
        assert(index < attributes.size());
        return attributes[index];
    }
};