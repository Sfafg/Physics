#pragma once
#include <unordered_map>
#include <span>
#include <glm/gtc/matrix_transform.hpp>
#include "Components.h"
#include "Material.h"
#include "VG/VG.h"
#include "BufferArray.h"
#include "DepthPyramid.h"

/// @brief System renderowania kierowany przez GPU
/// @details Działa przez przesłanie wszystkich potrzebnych danych do pamieci GPU,
/// które samo przetwarza dane, wykonuje odrzucanie obiektów poza stożkiem widoczności, oraz odrzcanie obiektów zasłoniętych.
/// @ref DepthPyramid
class GPUDrivenRendererSystem : public ECS::System<MeshArray>
{
    struct RenderObject
    {
        uint32_t objectID;
        uint32_t batchID;
        uint32_t meshID;
        RenderObject(uint32_t objectID = 0, uint32_t batchID = 0, uint32_t meshID = 0)
            : objectID(objectID), batchID(batchID), meshID(meshID)
        {}
    };

    struct MeshReferance
    {
        uint32_t vertexStart;
        uint32_t triangleStart;
        uint32_t triangleCount;
        MeshReferance(uint32_t vertexStart, uint32_t triangleStart, uint32_t triangleCount)
            :vertexStart(vertexStart), triangleStart(triangleStart), triangleCount(triangleCount)
        {}
    };

    struct BoundingBox
    {
        glm::vec3 min;
        glm::vec3 max;
        BoundingBox(glm::vec3 min = { FLT_MAX,FLT_MAX,FLT_MAX }, glm::vec3 max = { -FLT_MAX,-FLT_MAX ,-FLT_MAX }) :min(min), max(max) {}
    };

    struct Batch
    {
        uint32_t meshID;
        uint32_t objectCount;
        Batch(uint32_t meshID = 0, uint32_t objectCount = 0) :meshID(meshID), objectCount(objectCount) {}
    };

public:
    static vg::RenderPass renderPass;

private:
    enum class BufferUpdate
    {
        None = 0,
        Indices = 1,
        Vertices = 2,
        Normals = 4,
        Uvs = 8,
        Referances = 16,
        BoundingBoxes = 32,
        Mesh = Indices | Vertices | Normals | Uvs | Referances | BoundingBoxes,
        RenderObjects = 64,
        All = Mesh | RenderObjects
    };

    static vg::RenderPass depthPrepass;
    static vg::ComputePipeline computeRenderer;
    static vg::ComputePipeline clearInstructions;
    static const vg::Queue* queue;

    vg::DescriptorPool descriptorPool;

    BufferArray<uint32_t> indicesBuffer;
    BufferArray<glm::vec3> verticesBuffer;
    BufferArray<glm::vec3> normalsBuffer;
    BufferArray<glm::vec2> uvsBuffer;
    std::vector<MeshReferance> meshReferances;
    BufferArray<BoundingBox> meshBoundingBoxes;

    std::unordered_map<int, int> batchMap;
    std::vector<Batch> batches;
    BufferArray<uint32_t> batchMaterial;
    BufferArray<RenderObject> objectsBuffer;
    BufferArray<glm::mat4> instanceBuffer;
    BufferArray<uint32_t> mappingBuffer;
    BufferArray<vg::cmd::DrawIndexed> instructionsBuffer;
    DepthPyramid depthPyramid;
    BufferArray<glm::mat4> cameraMatrices;
    vg::Framebuffer depthPrepassFramebuffer;
    vg::DescriptorSet computeDescriptor;
    vg::DescriptorSet depthPrepassDescriptor;
    vg::DescriptorSet renderDescriptor;
    vg::DescriptorSet instructionDescriptors;
    vg::QueryPool query;
    vg::Flags<BufferUpdate> dataChanged;

    friend class Renderer;
public:
    GPUDrivenRendererSystem();
    GPUDrivenRendererSystem(std::span<Material> materials, Span<const vg::Attachment> attachments, const vg::Image& depthImage, const vg::ImageView& depthView, const vg::Queue* queue);

    GPUDrivenRendererSystem(GPUDrivenRendererSystem&& rhs) noexcept;
    GPUDrivenRendererSystem& operator = (GPUDrivenRendererSystem&& rhs);
    GPUDrivenRendererSystem(const GPUDrivenRendererSystem& rhs) = delete;
    GPUDrivenRendererSystem& operator = (const GPUDrivenRendererSystem& rhs) = delete;
    ~GPUDrivenRendererSystem();

    void CopyData(GPUDrivenRendererSystem& rendererSystem);

    uint32_t AddMesh(Span<const uint32_t> triangles, Span<const glm::vec3> vertices, Span<const glm::vec3> normals, Span<const glm::vec2> uvs);
    std::span<glm::vec3> GetMeshVertices(uint32_t id);
    void RemoveMesh();

    void ReserveRenderObjects(int count);
    void AddRenderObject(uint32_t mesh, uint32_t material, uint32_t variant, const glm::mat4& matrix);
    void RemoveRenderObject(uint32_t id);

    void GetRenderCommands(const vg::Image& depthImage, const vg::ImageView& depthView, vg::CmdBuffer& buffer, const glm::mat4& cameraView, const glm::mat4& cameraProjection, const vg::Framebuffer& framebuffer, uint32_t width, uint32_t height);
};