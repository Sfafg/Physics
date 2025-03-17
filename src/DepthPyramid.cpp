#include "DepthPyramid.h"
#include <glm/glm.hpp>

DepthPyramid::DepthPyramid(uint32_t width, uint32_t height)
{
    vg::Shader reductionShader(vg::ShaderStage::Compute, "resources/shaders/Reduction.comp.spv");
    computeReduction = vg::ComputePipeline(
        reductionShader,
        {
            {
                {0,vg::DescriptorType::CombinedImageSampler,1,vg::ShaderStage::Compute},
                {1,vg::DescriptorType::StorageImage,1,vg::ShaderStage::Compute}
            }
        },
        {
            vg::PushConstantRange(vg::ShaderStage::Compute, 0, sizeof(glm::ivec2))
        }
    );
    pool = vg::DescriptorPool(16, { { vg::DescriptorType::CombinedImageSampler, 16} ,{ vg::DescriptorType::StorageImage, 16} });
    reductionCmdBuffer = vg::CmdBuffer(vg::currentDevice->GetQueue(0), false, vg::CmdBufferLevel::Secondary);
}

vg::cmd::ExecuteCommands DepthPyramid::Generate(
    const vg::Image& depth,
    const vg::ImageView& depthView)
{
    uint32_t width = depth.GetDimensions()[0] / 2, height = depth.GetDimensions()[1] / 2;
    if (pyramidImage.GetDimensions()[0] != width || pyramidImage.GetDimensions()[1] != height)
    {
        // Prepare the pyramid resources.
        int mipLevels = int(std::floor(std::log2(std::max(width, height)))) + 1;
        pyramidImage = vg::Image({ width,height }, vg::Format::R32SFLOAT, { vg::ImageUsage::Storage, vg::ImageUsage::Sampled }, mipLevels);
        vg::Allocate(pyramidImage, { vg::MemoryProperty::DeviceLocal });
        pyramidImageView = vg::ImageView(pyramidImage, vg::ImageSubresource(vg::ImageAspect::Color, 0, mipLevels));
        reductionSampler = vg::Sampler(
            vg::Filter::Linear, vg::Filter::Linear,
            vg::SamplerMipmapMode::Nearest,
            vg::SamplerAddressMode::ClampToEdge,
            vg::SamplerAddressMode::ClampToEdge,
            vg::SamplerAddressMode::ClampToEdge,
            0.0f, 0.0f, mipLevels,
            vg::SamplerReduction::Max
        );
        mipImageViews.resize(mipLevels);
        descriptors.reserve(mipLevels);
        pool.Allocate(std::vector<vg::DescriptorSetLayoutHandle>(mipLevels, computeReduction.GetPipelineLayout().GetDescriptorSets()[0]), &descriptors);

        // Record the commands for pyramid creation.
        reductionCmdBuffer.Clear().Begin({}, vg::RenderPass(), 0, vg::Framebuffer()).Append(vg::cmd::BindPipeline(computeReduction));
        uint32_t inputWidth = depth.GetDimensions()[0], inputHeight = depth.GetDimensions()[1];
        for (int i = 0; i < mipLevels; i++)
        {
            mipImageViews[i] = vg::ImageView(pyramidImage, vg::ImageSubresource(vg::ImageAspect::Color, i, 1));
            const vg::ImageView& previousView = i == 0 ? depthView : mipImageViews[i - 1];
            descriptors[i].AttachImage(vg::DescriptorType::CombinedImageSampler, vg::ImageLayout::ShaderReadOnlyOptimal, previousView, reductionSampler, 0, 0);
            descriptors[i].AttachImage(vg::DescriptorType::StorageImage, vg::ImageLayout::General, mipImageViews[i], reductionSampler, 1, 0);

            vg::ImageMemoryBarrier previousImageBarrier;
            if (i == 0)
                previousImageBarrier = vg::ImageMemoryBarrier(depth, vg::ImageLayout::DepthStencilAttachmentOptimal, vg::ImageLayout::ShaderReadOnlyOptimal, vg::Access::ShaderWrite, vg::Access::ShaderRead, vg::ImageSubresource(vg::ImageAspect::Depth, 0, 1));
            else
                previousImageBarrier = vg::ImageMemoryBarrier(pyramidImage, vg::ImageLayout::General, vg::ImageLayout::ShaderReadOnlyOptimal, 0, vg::Access::ShaderRead, vg::ImageSubresource(vg::ImageAspect::Color, i - 1, 1));

            reductionCmdBuffer.Append(
                vg::cmd::PipelineBarier(vg::PipelineStage::ComputeShader, vg::PipelineStage::ComputeShader,
                    {
                    previousImageBarrier,
                    vg::ImageMemoryBarrier(pyramidImage,vg::ImageLayout::General, vg::Access::ShaderWrite, vg::Access::ShaderRead, vg::ImageSubresource(vg::ImageAspect::Color, i, 1))
                    }
                ),
                vg::cmd::BindDescriptorSets(computeReduction.GetPipelineLayout(), vg::PipelineBindPoint::Compute, 0, { descriptors[i] }),
                vg::cmd::PushConstants(computeReduction.GetPipelineLayout(), vg::ShaderStage::Compute, 0, std::make_tuple(inputHeight, inputWidth)),
                vg::cmd::Dispatch(std::ceil(width / 16.0f), std::ceil(height / 16.0f), 1)
            );
            inputWidth = width;
            inputHeight = height;
            width = std::max({ width / 2, 1U });
            height = std::max({ height / 2, 1U });
        }
        reductionCmdBuffer.Append(
            vg::cmd::PipelineBarier(vg::PipelineStage::ComputeShader, vg::PipelineStage::ComputeShader,
                {
                vg::ImageMemoryBarrier(pyramidImage, vg::ImageLayout::General,vg::ImageLayout::ShaderReadOnlyOptimal, vg::Access::ShaderWrite, vg::Access::ShaderRead, vg::ImageSubresource(vg::ImageAspect::Color, mipLevels - 1, 1))
                }
            )
        ).End();
    }

    return vg::cmd::ExecuteCommands({ reductionCmdBuffer });
}