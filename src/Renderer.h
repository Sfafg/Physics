#pragma once
#include "Components.h"

class Material
{
    vg::Subpass subpass;
    vg::SubpassDependency dependency;
public:
    Material(vg::Subpass&& subpass, const vg::SubpassDependency& dependency);

    friend class Renderer;
};

struct DescriptorAllocator
{
    static std::vector<vg::DescriptorPool> pools;

    static void Init();
    static std::vector<vg::DescriptorSet> Allocate(Span<const vg::DescriptorSetLayoutHandle> setLayouts);
    static void Destroy();
};

class Renderer : public ECS::System<MeshArray>
{
    struct Frame
    {
        vg::Framebuffer framebuffer;
        vg::CmdBuffer commandBuffer;
        std::vector<vg::CmdBuffer> secondaryBuffers;
        vg::Semaphore renderFinishedSemaphore;
        vg::Semaphore imageAvailableSemaphore;
        vg::Fence inFlightFence;
        BufferArray cameraBuffer;
        vg::DescriptorSet cameraDescriptor;
    };
    struct MeshDrawData
    {
        Mesh mesh;
        BufferArray buffer;
        vg::DescriptorSet descriptor;
        int frameIndex;
    };

public:
    static BufferArray uniformBufferPool;
    static std::vector<Material> materials;
    static vg::Surface surface;
    static vg::Swapchain swapchain;
    static vg::RenderPass renderPass;
    static DescriptorAllocator descriptorAllocator;
    static vg::Image depthImage;
    static vg::ImageView depthImageView;
    static int frameIndex;
    static std::vector<Frame> frameData;
    static std::vector<MeshDrawData> tempMeshDrawData;

    static void Add(MeshArray& component);

    static void Destroy(MeshArray& component);

    static void Init(vg::SurfaceHandle windowSurface, int width, int height, vg::Queue& queue);

    template <typename... T>
    static void DrawMesh(Mesh mesh, const std::tuple<T...>& uniformData, int frameCount = 1)
    {
        auto buffer = BufferArray(std::vector<std::tuple<T...>>{ uniformData }, vg::BufferUsage::UniformBuffer, vg::MemoryProperty::HostVisible);
        vg::DescriptorSet descriptor = std::move(descriptorAllocator.Allocate(renderPass.GetPipelineLayouts()[mesh.materialID].GetDescriptorSets()[1])[0]);
        descriptor.AttachBuffer(vg::DescriptorType::UniformBuffer, buffer, 0, ((vg::Buffer&) buffer).GetSize(), 0, 0);

        tempMeshDrawData.push_back({ mesh,buffer,std::move(descriptor),-frameCount });
    }

    static void Draw(Mesh& mesh, vg::DescriptorSet& descriptor);

    static void Run(Transform& cameraTransform, float fov, vg::Queue& queue, bool resize, int width, int height);

    static void Free();

    struct CameraBuffer
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 lightPosition;
    };

private:
    static void InitRenderPass();

    static void SwapchainResize(vg::Swapchain& swapchain, vg::Swapchain& oldSwapchain, int width, int height);
};

REGISTER_SYSTEMS(MeshArray, Renderer);