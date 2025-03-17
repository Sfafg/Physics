#pragma once
#include <vector>
#include "VG/Span.h"
#include "Material.h"
#include "VG/VG.h"

/// @brief Struktura odpowiedzialna za zarządanie danymi siatki obiektu. 
struct Mesh
{
    std::vector<vg::Buffer> attributes;
    vg::Buffer triangleBuffer;
    vg::IndexType indexType;
    uint32_t materialID;

    Mesh();
    Mesh(Span<const uint64_t> attributesSize, uint32_t materialID);
    Mesh(vg::IndexType indexType, uint32_t triangleCount, Span<const uint64_t> attributesSize, uint32_t materialID);

    void WriteTriangles(void* data, uint64_t size, uint64_t offset = 0);
    void ReadTriangles(void*& data, uint64_t size, uint64_t offset = 0);
    void WriteAttribute(uint32_t attributeIndex, void* data, uint64_t size, uint64_t offset = 0);
    void ReadAttribute(uint32_t attributeIndex, void*& data, uint64_t size, uint64_t offset = 0);

    template <typename T>
    void WriteTriangles(Span<const T> data, uint64_t offset = 0)
    {
        return WriteTriangles((void*) data.data(), data.size() * sizeof(T), offset * sizeof(T));
    }

    template <typename T>
    void ReadTriangles(std::vector<T>* data, uint64_t offset = 0)
    {
        if (data->size() == 0)
            data->resize(triangleBuffer.GetSize() / sizeof(T) - offset);

        void* d;
        ReadTriangles(d, data->size() * sizeof(T), offset * sizeof(T));
        memcpy(data->data(), d, data->size() * sizeof(T));
        free(d);
    }

    template <typename T>
    void WriteAttribute(uint32_t attributeIndex, Span<const T> data, uint64_t offset = 0)
    {
        return WriteAttribute(attributeIndex, (void*) data.data(), data.size() * sizeof(T), offset * sizeof(T));
    }

    template <typename T>
    void ReadAttribute(uint32_t attributeIndex, std::vector<T>* data, uint64_t offset = 0)
    {
        if (data->size() == 0)
            data->resize(attributes[attributeIndex].GetSize() / sizeof(T) - offset);

        void* d;
        ReadAttribute(attributeIndex, d, data->size() * sizeof(T), offset * sizeof(T));
        memcpy(data->data(), d, data->size() * sizeof(T));
        free(d);
    }
};