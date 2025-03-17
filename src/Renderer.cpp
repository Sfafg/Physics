#include "Renderer.h"
#include "GLFW/glfw3.h"
#include <unordered_map>
void Renderer::Init(void* window, vg::SurfaceHandle windowSurface, int width, int height)
{
    frameIndex = 0;
    Renderer::window = window;
    surface = vg::Surface(windowSurface, { vg::Format::BGRA8SRGB, vg::ColorSpace::SRGBNL });
    swapchain = vg::Swapchain(surface, 2, width, height, vg::Usage::ColorAttachment, vg::PresentMode::Fifo);

    depthImage.resize(swapchain.GetImageCount());
    depthImageView.resize(swapchain.GetImageCount());
    framebuffers.resize(swapchain.GetImageCount());
    renderSystem.resize(swapchain.GetImageCount());
    for (int i = 0; i < framebuffers.size(); i++)
    {
        depthImage[i] = vg::Image({ swapchain.GetWidth(), swapchain.GetHeight() }, { vg::Format::D32SFLOAT,vg::Format::D32SFLOATS8UINT,vg::Format::x8D24UNORMPACK }, { vg::FormatFeature::DepthStencilAttachment }, { vg::ImageUsage::DepthStencilAttachment,vg::ImageUsage::Sampled });
        vg::Allocate({ &depthImage[i] }, { vg::MemoryProperty::DeviceLocal });
        depthImageView[i] = vg::ImageView(depthImage[i], { vg::ImageAspect::Depth });
        renderSystem[i] = GPUDrivenRendererSystem(materials, { vg::Attachment(surface.GetFormat(),vg::ImageLayout::PresentSrc) , vg::Attachment(depthImage[0].GetFormat(),vg::ImageLayout::DepthStencilAttachmentOptimal) }, depthImage[i], depthImageView[i], &vg::currentDevice->GetQueue(0));
    }

    for (int i = 0; i < framebuffers.size(); i++)
        framebuffers[i] = vg::Framebuffer(GPUDrivenRendererSystem::renderPass, { swapchain.GetImageViews()[i],depthImageView[i] }, swapchain.GetWidth(), swapchain.GetHeight());

    commandBuffer.resize(swapchain.GetImageCount());
    renderFinishedSemaphore.resize(swapchain.GetImageCount());
    imageAvailableSemaphore.resize(swapchain.GetImageCount());
    inFlightFence.resize(swapchain.GetImageCount());

    for (int i = 0; i < swapchain.GetImageCount(); i++)
    {
        commandBuffer[i] = vg::CmdBuffer(vg::currentDevice->GetQueue(0));
        renderFinishedSemaphore[i] = vg::Semaphore();
        imageAvailableSemaphore[i] = vg::Semaphore();
        inFlightFence[i] = vg::Fence(true);
    }
}

void Renderer::Add(MeshArray& component)
{
    renderSystem[frameIndex].AddRenderObject(component.meshID, component.materialID, component.materialVariantID, component.GetComponent<Transform>().Matrix());
}
void Renderer::Destroy(MeshArray& component)
{

}

void Renderer::DrawFrame(Transform cameraTransform, float fov)
{
    vg::currentDevice->WaitUntilIdle();
    inFlightFence[frameIndex].Await(true);

    vg::Swapchain oldSwapchain;
    int width, height;
    glfwGetFramebufferSize((GLFWwindow*) window, &width, &height);
    if (width != swapchain.GetWidth() || height != swapchain.GetHeight())
    {
        while (width != 0 && height != 0)
        {
            glfwGetFramebufferSize((GLFWwindow*) window, &width, &height);
            glfwPollEvents();
        }
        vg::currentDevice->WaitUntilIdle();

        std::swap(oldSwapchain, swapchain);
        swapchain = vg::Swapchain(surface, 2, width, height, oldSwapchain);
        depthImage.resize(swapchain.GetImageCount());
        depthImageView.resize(swapchain.GetImageCount());
        for (int i = 0; i < framebuffers.size(); i++)
        {
            depthImage[i] = vg::Image({ swapchain.GetWidth(), swapchain.GetHeight() }, depthImage[i].GetFormat(), { vg::ImageUsage::DepthStencilAttachment, vg::ImageUsage::Sampled });
            vg::Allocate(Span<vg::Image* const>{&depthImage[i] }, { vg::MemoryProperty::DeviceLocal });
            depthImageView[i] = vg::ImageView(depthImage[i], { vg::ImageAspect::Depth });
            framebuffers[i] = vg::Framebuffer(GPUDrivenRendererSystem::renderPass, { swapchain.GetImageViews()[i], depthImageView[i] }, swapchain.GetWidth(), swapchain.GetHeight());
        }
    }

    auto [imageIndex, result] = swapchain.GetNextImageIndex(imageAvailableSemaphore[frameIndex]);
    vg::currentDevice->WaitUntilIdle();

    glm::mat4 cameraView = glm::lookAt(cameraTransform.position, cameraTransform.position + cameraTransform.Forward(), cameraTransform.Up());
    glm::mat4 cameraProjection = glm::perspective(fov, (float) width / (float) height, 0.1f, 1000.0f);
    cameraProjection[1][1] *= -1;

    int prevFrame = frameIndex - 1;
    if (prevFrame < 0) prevFrame = swapchain.GetImageCount() + prevFrame;
    renderSystem[frameIndex].CopyData(renderSystem[prevFrame]);

    commandBuffer[frameIndex].Clear().Begin();
    renderSystem[frameIndex].GetRenderCommands(depthImage[frameIndex], depthImageView[frameIndex], commandBuffer[frameIndex], cameraView, cameraProjection, framebuffers[frameIndex], width, height);
}

void Renderer::Present(vg::Queue& queue)
{
    commandBuffer[frameIndex].Append(vg::cmd::EndRenderpass());
    commandBuffer[frameIndex].End().Submit({ {vg::PipelineStage::ColorAttachmentOutput, imageAvailableSemaphore[frameIndex]} }, { renderFinishedSemaphore[frameIndex] }, inFlightFence[frameIndex]);
    queue.Present({ renderFinishedSemaphore[frameIndex] }, { swapchain }, { (uint32_t) frameIndex });
    frameIndex = (frameIndex + 1) % swapchain.GetImageCount();
}

void Renderer::Destroy()
{
    swapchain.~Swapchain();

    framebuffers.clear();
    depthImage.clear();
    depthImageView.clear();
    commandBuffer.clear();
    renderFinishedSemaphore.clear();
    imageAvailableSemaphore.clear();
    inFlightFence.clear();
    renderSystem.clear();

    materials.clear();
}


vg::Surface Renderer::surface;
vg::Swapchain Renderer::swapchain;
std::vector<vg::Image> Renderer::depthImage;
std::vector<vg::ImageView> Renderer::depthImageView;
std::vector<vg::Framebuffer> Renderer::framebuffers;
int Renderer::frameIndex;

std::vector<vg::CmdBuffer> Renderer::commandBuffer;
std::vector<vg::Semaphore> Renderer::renderFinishedSemaphore;
std::vector<vg::Semaphore> Renderer::imageAvailableSemaphore;
std::vector<vg::Fence> Renderer::inFlightFence;

std::vector<GPUDrivenRendererSystem> Renderer::renderSystem;

void* Renderer::window;
std::vector<Material> Renderer::materials;