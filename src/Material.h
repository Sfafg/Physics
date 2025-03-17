#pragma once
#include <vector>
#include <memory>
#include "VG/VG.h"
#include "VG/Span.h"
#include "BufferArray.h"
#include "glm/glm.hpp"

/// @brief Dane wariantu materiału.
struct VariantData
{
    float r;
    float g;
    float b;
};

/// @brief Struktura trzymająca dane materiału oraz jego wariantów.
struct Material
{
    std::unique_ptr<std::vector<vg::Shader*>> shaders;
    std::unique_ptr<vg::InputAssembly> inputAssembly;
    std::unique_ptr<vg::Rasterizer> rasterizer;
    std::unique_ptr<vg::Multisampling> multisampling;
    std::unique_ptr<vg::DepthStencil> depthStencil;
    std::unique_ptr<vg::ColorBlending> colorBlending;
    std::unique_ptr<std::vector<vg::DynamicState>> dynamicStates;
    BufferArray<VariantData> variants;

    Material() {}
    Material(Span<vg::Shader* const> shaders, vg::InputAssembly&& inputAssembly, vg::Rasterizer&& rasterizer, vg::Multisampling&& multisampling, vg::DepthStencil&& depthStencil, vg::ColorBlending&& colorBlending, Span<const vg::DynamicState> dynamicStates)
        :
        shaders(new std::vector<vg::Shader*>(shaders.begin(), shaders.end())), inputAssembly(new auto(inputAssembly)),
        rasterizer(new auto(rasterizer)), multisampling(new auto(multisampling)), depthStencil(new auto(depthStencil)),
        colorBlending(new auto(colorBlending)), dynamicStates(new std::vector<vg::DynamicState>(dynamicStates.begin(), dynamicStates.end())),
        variants(BufferArray<VariantData>(0, vg::BufferUsage::StorageBuffer))
    {}

    void AddVariant(VariantData&& variant)
    {
        variants.push_back(std::move(variant));
    }
};
