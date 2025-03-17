#pragma once
#include <vector>
#include "VG/VG.h"
#include "BufferArray.h"
#include <glm/gtc/matrix_transform.hpp>


/// @brief Klasa przechowująca dane i generująca piramidę głębokości dla bufera głębokości.
/// @details Piramida głębokości jest podobna do łańcucha mip map, natomiast w kolejnych poziomach bierze wartość maksymalną z sąsiadów.
class DepthPyramid
{
    vg::CmdBuffer reductionCmdBuffer;

public:
    vg::ComputePipeline computeReduction;
    vg::Image pyramidImage;
    vg::ImageView pyramidImageView;
    std::vector<vg::ImageView> mipImageViews;
    vg::Sampler reductionSampler;
    vg::DescriptorPool pool;
    std::vector<vg::DescriptorSet> descriptors;

public:

    DepthPyramid() {}
    DepthPyramid(uint32_t width, uint32_t height);

    /// @brief Funkcja zwracająca komendę, która wygeneruje piramidę głębokości.
    /// @param depth obraz głębokości
    /// @param depthView opis obrazu głębokości
    /// @return komenda generująca
    vg::cmd::ExecuteCommands Generate(
        const vg::Image& depth,
        const vg::ImageView& depthView);
};