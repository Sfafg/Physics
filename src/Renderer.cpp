#include "Renderer.h"
#include <chrono>
#include <thread>

Material::Material(vg::Subpass&& subpass, const vg::SubpassDependency& dependency)
    : subpass(std::move(subpass)), dependency(dependency)
{}

static std::vector<vg::DescriptorPool> pools;

void DescriptorAllocator::Init()
{
    pools.push_back(vg::DescriptorPool(4096, { {vg::DescriptorType::UniformBuffer, 4096} }));
}

std::vector<vg::DescriptorSet> DescriptorAllocator::Allocate(Span<const vg::DescriptorSetLayoutHandle> setLayouts)
{
    for (int i = 0; i < pools.size(); i++)
    {
        try
        {
            return pools[i].Allocate(setLayouts);
        }
        catch (...) {};
    }

    pools.resize(pools.size() + 1);
    return Allocate(setLayouts);
}
void DescriptorAllocator::Destroy()
{
    pools.resize(0);
}
std::vector<vg::DescriptorPool> DescriptorAllocator::pools;

void Renderer::Add(MeshArray& component)
{
    BufferArray& buffer = component.dynamicBuffer;
    vg::DescriptorSet& descriptor = component.dynamicBufferDescriptorSet;

    buffer = BufferArray(sizeof(glm::mat4), vg::BufferUsage::UniformBuffer, vg::MemoryProperty::HostVisible);
    descriptor = std::move(descriptorAllocator.Allocate(renderPass.GetPipelineLayouts()[component.meshes[0].materialID].GetDescriptorSets()[1])[0]);
    descriptor.AttachBuffer(vg::DescriptorType::UniformBuffer, buffer, 0, ((vg::Buffer&) buffer).GetSize(), 0, 0);
}

void Renderer::Destroy(MeshArray& component)
{
    for (int i = 0; i < component.meshes.size(); i++)
        tempMeshDrawData.push_back({ component.meshes[i], component.dynamicBuffer,std::move(component.dynamicBufferDescriptorSet),-1 });
}

void Renderer::Init(vg::SurfaceHandle windowSurface, int width, int height, vg::Queue& queue)
{
    frameIndex = 0;
    surface = vg::Surface(windowSurface, { vg::Format::BGRA8SRGB, vg::ColorSpace::SRGBNL });
    swapchain = vg::Swapchain(surface, 2, width, height);
    depthImage = vg::Image({ swapchain.GetWidth(), swapchain.GetHeight() }, { vg::Format::D32SFLOAT,vg::Format::D32SFLOATS8UINT,vg::Format::x8D24UNORMPACK }, { vg::FormatFeature::DepthStencilAttachment }, { vg::ImageUsage::DepthStencilAttachment });
    vg::Allocate({ &depthImage }, { vg::MemoryProperty::DeviceLocal });
    depthImageView = vg::ImageView(depthImage, { vg::ImageAspect::Depth });

    InitRenderPass();

    uint32_t uniformBufferCount = swapchain.GetImageCount();
    descriptorAllocator.Init();

    frameData.resize(swapchain.GetImageCount());
    for (int i = 0; i < frameData.size(); i++)
    {
        frameData[i].framebuffer = vg::Framebuffer(renderPass, { swapchain.GetImageViews()[i],depthImageView }, swapchain.GetWidth(), swapchain.GetHeight());
        frameData[i].commandBuffer = vg::CmdBuffer(queue);
        frameData[i].secondaryBuffers.resize(materials.size());
        frameData[i].renderFinishedSemaphore = vg::Semaphore();
        frameData[i].imageAvailableSemaphore = vg::Semaphore();
        frameData[i].inFlightFence = vg::Fence(true);
        frameData[i].cameraBuffer = BufferArray(sizeof(CameraBuffer), vg::BufferUsage::UniformBuffer, vg::MemoryProperty::HostVisible);
        frameData[i].cameraDescriptor = std::move(descriptorAllocator.Allocate(renderPass.GetPipelineLayouts()[0].GetDescriptorSets()[0])[0]);
        frameData[i].cameraDescriptor.AttachBuffer(vg::DescriptorType::UniformBuffer, frameData[i].cameraBuffer, 0, sizeof(CameraBuffer), 0, 0);

        for (int j = 0; j < materials.size(); j++)
            frameData[i].secondaryBuffers[j] = vg::CmdBuffer(queue, false, vg::CmdBufferLevel::Secondary);
    }
}

void Renderer::Draw(Mesh& mesh, vg::DescriptorSet& descriptor)
{
    std::vector<vg::BufferHandle> attributeHandles(mesh.attributes.size());
    for (int j = 0; j < attributeHandles.size(); j++)
        attributeHandles[j] = mesh.GetAttribute(j);

    frameData[frameIndex].secondaryBuffers[mesh.materialID].Append(
        vg::cmd::BindDescriptorSets(renderPass.GetPipelineLayouts()[mesh.materialID], vg::PipelineBindPoint::Graphics, 0, { frameData[frameIndex].cameraDescriptor, descriptor }),
        vg::cmd::BindVertexBuffers(attributeHandles)
    );
    if (mesh.indexData.buffer)
        frameData[frameIndex].secondaryBuffers[mesh.materialID].Append(
            vg::cmd::BindIndexBuffer(mesh.indexData, 0, mesh.indexType),
            vg::cmd::DrawIndexed(mesh.indexCount)
        );

    else
        frameData[frameIndex].secondaryBuffers[mesh.materialID].Append(
            vg::cmd::Draw(mesh.vertexCount)
        );

}

void Renderer::Run(Transform& cameraTransform, float fov, vg::Queue& queue, bool resize, int width, int height)
{
    static auto start = std::chrono::high_resolution_clock::now();

    Frame& currentFrame = frameData[frameIndex];
    currentFrame.inFlightFence.Await(true);

    vg::Swapchain oldSwapchain;
    if (resize)
    {
        std::swap(oldSwapchain, swapchain);
        SwapchainResize(swapchain, oldSwapchain, width, height);
    }

    auto [imageIndex, result] = swapchain.GetNextImageIndex(currentFrame.imageAvailableSemaphore);

    std::chrono::duration<float, std::ratio<1, 1>> time = std::chrono::high_resolution_clock::now() - start;

    CameraBuffer ubo;
    ubo.view = glm::lookAt(cameraTransform.position, cameraTransform.position + cameraTransform.Forward(), cameraTransform.Up());
    ubo.proj = glm::perspective(glm::radians(fov), swapchain.GetWidth() / (float) swapchain.GetHeight(), 0.01f, 400.0f);
    ubo.proj[1][1] *= -1;
    ubo.lightPosition = { 0,0,50 };
    currentFrame.cameraBuffer.Write(std::vector<CameraBuffer>{ ubo });

    for (int i = 0; i < materials.size(); i++)
    {
        currentFrame.secondaryBuffers[i]
            .Begin({ vg::CmdBufferUsage::RenderPassContinue }, renderPass, i, currentFrame.framebuffer)
            .Append(
                vg::cmd::BindPipeline(renderPass.GetPipelines()[i]),
                vg::cmd::SetViewport(vg::Viewport(swapchain.GetWidth(), swapchain.GetHeight())),
                vg::cmd::SetScissor(vg::Scissor(swapchain.GetWidth(), swapchain.GetHeight()))
            );
    }
    for (int i = tempMeshDrawData.size() - 1; i >= 0; i--)
    {
        if (tempMeshDrawData[i].frameIndex < 0)
        {
            if (tempMeshDrawData[i].frameIndex != -1)
                tempMeshDrawData[i].frameIndex++;
            else
                tempMeshDrawData[i].frameIndex = frameIndex;

            Draw(tempMeshDrawData[i].mesh, tempMeshDrawData[i].descriptor);
        }
        else if (tempMeshDrawData[i].frameIndex == frameIndex)
        {
            tempMeshDrawData.erase(tempMeshDrawData.begin() + i);
        }
    }

    for (auto&& component : components)
    {
        component.dynamicBuffer.Write(std::vector<glm::mat4>{ component.GetComponent<Transform>().Matrix() }, 0);

        for (auto&& mesh : component.meshes)
            Draw(mesh, component.dynamicBufferDescriptorSet);
    }
    for (auto&& secondaryBuffer : currentFrame.secondaryBuffers)
        secondaryBuffer.End();

    currentFrame.commandBuffer.Clear().Begin({}).Append(
        vg::cmd::BeginRenderpass(renderPass, currentFrame.framebuffer, { 0, 0 }, { swapchain.GetWidth(), swapchain.GetHeight() }, { vg::ClearColor{ 0,0,0,255 },vg::ClearDepthStencil{1.0f,0U} }, vg::SubpassContents::SecondaryCommandBuffers)
    );
    for (int j = 0; j < materials.size() - 1; j++)
    {
        currentFrame.commandBuffer.Append(
            vg::cmd::ExecuteCommands({ currentFrame.secondaryBuffers[j] }),
            vg::cmd::NextSubpass(vg::SubpassContents::SecondaryCommandBuffers)
        );
    }
    currentFrame.commandBuffer.Append(
        vg::cmd::ExecuteCommands({ currentFrame.secondaryBuffers[materials.size() - 1] }),
        vg::cmd::EndRenderpass()
    ).End();

    currentFrame.commandBuffer.Submit({ {vg::PipelineStage::ColorAttachmentOutput, currentFrame.imageAvailableSemaphore} }, { currentFrame.renderFinishedSemaphore }, currentFrame.inFlightFence);
    queue.Present({ currentFrame.renderFinishedSemaphore }, { swapchain }, { imageIndex });

    frameIndex = (frameIndex + 1) % swapchain.GetImageCount();
}

void Renderer::Free()
{
    for (auto&& frame : frameData)
        frame.inFlightFence.Await();

    tempMeshDrawData.clear();
    materials.clear();
    components.clear();
    frameData.clear();
    depthImage.~Image();
    depthImageView.~ImageView();
    descriptorAllocator.Destroy();
    renderPass.~RenderPass();
    swapchain.~Swapchain();
    surface.~Surface();
}

struct CameraBuffer
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 lightPosition;
};

void Renderer::InitRenderPass()
{
    std::vector<vg::Subpass> subpasses(materials.size());
    std::vector<vg::SubpassDependency> subpassDependencies(materials.size());
    for (int i = 0; i < materials.size(); i++)
    {
        subpasses[i] = std::move(materials[i].subpass);
        subpassDependencies[i] = std::move(materials[i].dependency);
    }

    renderPass = vg::RenderPass(
        {
            vg::Attachment(surface.GetFormat(), vg::ImageLayout::PresentSrc),
            vg::Attachment(depthImage.GetFormat(),vg::ImageLayout::DepthStencilAttachmentOptimal)
        },
        subpasses,
        subpassDependencies
    );
}

void Renderer::SwapchainResize(vg::Swapchain& swapchain, vg::Swapchain& oldSwapchain, int width, int height)
{
    vg::currentDevice->WaitUntilIdle();

    swapchain = vg::Swapchain(surface, 2, width, height, oldSwapchain);
    depthImage = vg::Image({ swapchain.GetWidth(), swapchain.GetHeight() }, depthImage.GetFormat(), { vg::ImageUsage::DepthStencilAttachment });
    vg::Allocate({ &depthImage }, { vg::MemoryProperty::DeviceLocal });
    depthImageView = vg::ImageView(depthImage, { vg::ImageAspect::Depth });

    for (int i = 0; i < frameData.size(); i++)
        frameData[i].framebuffer = vg::Framebuffer(renderPass, { swapchain.GetImageViews()[i],depthImageView }, swapchain.GetWidth(), swapchain.GetHeight());
}

std::vector<Material> Renderer::materials;
vg::Surface Renderer::surface;
vg::Swapchain Renderer::swapchain;
vg::RenderPass Renderer::renderPass;
DescriptorAllocator Renderer::descriptorAllocator;
vg::Image Renderer::depthImage;
vg::ImageView Renderer::depthImageView;
int Renderer::frameIndex;
std::vector<Renderer::Frame> Renderer::frameData;
std::vector<Renderer::MeshDrawData> Renderer::tempMeshDrawData;