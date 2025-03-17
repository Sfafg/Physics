#pragma once
#include <vector>
#include "Components.h"
#include "Mesh.h"
#include "VG/VG.h"
#include "GPUDrivenRendererSystem.h"

/// @brief System renderowania, rozdzielający dane dla szczegółowych systemów renderowania.
class Renderer : public ECS::System<MeshArray>
{
    static void* window;
    static vg::Surface surface;
    static vg::Swapchain swapchain;

    static std::vector<vg::Framebuffer> framebuffers;
    static std::vector<vg::Image> depthImage;
    static std::vector<vg::ImageView> depthImageView;
    static std::vector<vg::CmdBuffer> commandBuffer;
    static std::vector<vg::Semaphore> renderFinishedSemaphore;
    static std::vector<vg::Semaphore> imageAvailableSemaphore;
    static std::vector<vg::Fence> inFlightFence;
    static int frameIndex;

public:
    static std::vector<GPUDrivenRendererSystem> renderSystem;
    static std::vector<Material> materials;
    static void Init(void* window, vg::SurfaceHandle windowSurface, int width, int height);

    static void Add(MeshArray& component);
    static void Destroy(MeshArray& component);

    static void DrawFrame(Transform cameraTransform, float fov);

    static void Present(vg::Queue& queue);

    static void Destroy();

    static vg::CmdBuffer& GetCurrentCmdBuffer() { return commandBuffer[frameIndex]; }
};
REGISTER_SYSTEMS(MeshArray, Renderer);