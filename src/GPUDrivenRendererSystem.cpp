#include <unordered_map>
#include "GPUDrivenRendererSystem.h"
#include <string>

vg::RenderPass GPUDrivenRendererSystem::renderPass;
vg::RenderPass GPUDrivenRendererSystem::depthPrepass;
vg::ComputePipeline GPUDrivenRendererSystem::computeRenderer;
vg::ComputePipeline GPUDrivenRendererSystem::clearInstructions;
const vg::Queue* GPUDrivenRendererSystem::queue = nullptr;
int objectCount = 0;

std::tuple<vg::Subpass, vg::SubpassDependency> CreateSubpass(Material&& material, int childrenCount)
{
    return std::make_tuple(
        vg::Subpass(
            vg::GraphicsPipeline(
                {
                    {
                        { 0,vg::DescriptorType::UniformBuffer,1,vg::ShaderStage::Vertex },
                        { 1,vg::DescriptorType::StorageBuffer,1,vg::ShaderStage::Vertex },
                        { 2,vg::DescriptorType::StorageBuffer,1,vg::ShaderStage::Vertex },
                        { 3,vg::DescriptorType::StorageBuffer,1,vg::ShaderStage::Vertex },
                        { 4,vg::DescriptorType::StorageBuffer,1,vg::ShaderStage::Fragment },
                    }
                },
                {},
                std::move(*material.shaders),
                {
                    { {0, sizeof(glm::vec3)},{1, sizeof(glm::vec3)},{2, sizeof(glm::vec2)}, {3,sizeof(int),vg::InputRate::Instance} },
                    { {0, 0, vg::Format::RGB32SFLOAT},{1, 1, vg::Format::RGB32SFLOAT},{2, 2, vg::Format::RG32SFLOAT}, {3,3,vg::Format::R32UINT} }
                },
                std::move(*material.inputAssembly), vg::Tesselation(), vg::ViewportState(vg::Viewport(), vg::Scissor()), std::move(*material.rasterizer),
                std::move(*material.multisampling), std::move(*material.depthStencil), std::move(*material.colorBlending), std::move(*material.dynamicStates),
                childrenCount
            ),
            {}, { vg::AttachmentReference(0, vg::ImageLayout::ColorAttachmentOptimal) },
            {}, vg::AttachmentReference(1, vg::ImageLayout::DepthStencilAttachmentOptimal)
        ),
        vg::SubpassDependency(-1, 0, vg::PipelineStage::ColorAttachmentOutput, vg::PipelineStage::ColorAttachmentOutput, 0, vg::Access::ColorAttachmentWrite, {})
    );
}
std::tuple<vg::Subpass, vg::SubpassDependency> CreateSecondarySubpass(Material&& material, int parentIndex)
{
    return std::make_tuple(vg::Subpass(
        vg::GraphicsPipeline(
            parentIndex, std::nullopt, std::nullopt,
            std::move(*material.shaders), std::nullopt,
            std::move(*material.inputAssembly), std::nullopt, std::nullopt, std::move(*material.rasterizer),
            std::move(*material.multisampling), std::move(*material.depthStencil),
            std::move(*material.colorBlending), std::move(*material.dynamicStates)
        ),
        {}, { vg::AttachmentReference(0, vg::ImageLayout::ColorAttachmentOptimal) },
        {}, vg::AttachmentReference(1, vg::ImageLayout::DepthStencilAttachmentOptimal)
    ),
        vg::SubpassDependency(-1, 0, vg::PipelineStage::ColorAttachmentOutput, vg::PipelineStage::ColorAttachmentOutput, 0, vg::Access::ColorAttachmentWrite, {})
    );
}

GPUDrivenRendererSystem::GPUDrivenRendererSystem() {}

GPUDrivenRendererSystem::GPUDrivenRendererSystem(std::span<Material> materials, Span<const vg::Attachment> attachments, const vg::Image& depthImage, const vg::ImageView& depthView, const vg::Queue* queue)
{
    objectCount++;
    if (!this->queue)
    {
        this->queue = queue;
        vg::Shader passShader(vg::ShaderStage::Vertex, "resources/shaders/passthrough.vert.spv");
        depthPrepass = vg::RenderPass(
            { attachments[1] },
        {
            vg::Subpass(
                vg::GraphicsPipeline(
                    {
                        {
                            { 0,vg::DescriptorType::UniformBuffer,1,vg::ShaderStage::Vertex },
                            { 1,vg::DescriptorType::StorageBuffer,1,vg::ShaderStage::Vertex }
                        }
                    },
                    {},
                    std::vector<vg::Shader*>{&passShader},
                    {
                        { {0, sizeof(glm::vec3)},{1, sizeof(glm::vec3)},{2, sizeof(glm::vec2)}, {3,sizeof(int),vg::InputRate::Instance} },
                        { {0, 0, vg::Format::RGB32SFLOAT},{1, 1, vg::Format::RGB32SFLOAT},{2, 2, vg::Format::RG32SFLOAT}, {3,3,vg::Format::R32UINT} }
                    },
                    vg::InputAssembly(vg::Primitive::Triangles),
                    vg::Tesselation(), vg::ViewportState(vg::Viewport(), vg::Scissor()),
                    vg::Rasterizer(false, false, vg::PolygonMode::Fill, vg::CullMode::Back, vg::FrontFace::Clockwise),
                    vg::Multisampling(1), vg::DepthStencil(true, true, vg::CompareOp::Less),
                    vg::ColorBlending(true, vg::LogicOp::Copy, { 0,0,0,0 }, {  }),
                    { vg::DynamicState::Viewport, vg::DynamicState::Scissor }
                ),
                {}, { },
                {}, vg::AttachmentReference(0, vg::ImageLayout::DepthStencilAttachmentOptimal)
            )
        },
        {
            vg::SubpassDependency(-1, 0, vg::PipelineStage::FragmentShader, vg::PipelineStage::FragmentShader, 0, vg::Access::ShaderWrite, vg::Dependency::ByRegion)
        }
        );

        std::vector<vg::Subpass> subpasses;
        std::vector<vg::SubpassDependency> subpassDependencies;
        subpasses.resize(subpasses.size() + materials.size());
        subpassDependencies.resize(subpassDependencies.size() + materials.size());
        for (int i = 0; i < materials.size(); i++)
        {
            if (i == 0)
                std::tie(subpasses[i], subpassDependencies[i]) = CreateSubpass(std::move(materials[i]), materials.size());
            else
                std::tie(subpasses[i], subpassDependencies[i]) = CreateSecondarySubpass(std::move(materials[i]), 0);
        }

        renderPass = vg::RenderPass(attachments, subpasses, subpassDependencies);

        vg::Shader renderShader(vg::ShaderStage::Compute, "resources/shaders/Renderer.comp.spv");
        computeRenderer = vg::ComputePipeline(
            renderShader,
            {
                {
                     {0,vg::DescriptorType::UniformBuffer,1,vg::ShaderStage::Compute},
                     {1,vg::DescriptorType::StorageBuffer,1,vg::ShaderStage::Compute},
                     {2,vg::DescriptorType::StorageBuffer,1,vg::ShaderStage::Compute},
                     {3,vg::DescriptorType::StorageBuffer,1,vg::ShaderStage::Compute},
                     {4,vg::DescriptorType::StorageBuffer,1,vg::ShaderStage::Compute},
                     {5,vg::DescriptorType::StorageBuffer,1,vg::ShaderStage::Compute},
                     {6,vg::DescriptorType::CombinedImageSampler,1,vg::ShaderStage::Compute}
                }
            },
        {
            vg::PushConstantRange(vg::ShaderStage::Compute,0, sizeof(uint32_t) * 3)
        }
        );

        vg::Shader clearShader(vg::ShaderStage::Compute, "resources/shaders/ClearInstructions.comp.spv");
        clearInstructions = vg::ComputePipeline(
            clearShader,
            {
                {
                    {0,vg::DescriptorType::StorageBuffer, 1, vg::ShaderStage::Compute}
                }
            },
        {
            vg::PushConstantRange(vg::ShaderStage::Compute, 0, sizeof(uint32_t))
        }
        );
    }

    dataChanged = false;
    descriptorPool = vg::DescriptorPool(36,
        {
            { vg::DescriptorType::UniformBuffer, 2 },
            { vg::DescriptorType::StorageBuffer, 4 },
            { vg::DescriptorType::CombinedImageSampler, 16 },
            { vg::DescriptorType::SampledImage, 16 }
        });
    descriptorPool.Allocate({ computeRenderer.GetPipelineLayout().GetDescriptorSets()[0] }, & computeDescriptor);
    descriptorPool.Allocate({ renderPass.GetPipelineLayouts()[0].GetDescriptorSets()[0] }, & renderDescriptor);
    descriptorPool.Allocate({ clearInstructions.GetPipelineLayout().GetDescriptorSets()[0] }, & instructionDescriptors);
    descriptorPool.Allocate({ depthPrepass.GetPipelineLayouts()[0].GetDescriptorSets()[0] }, & depthPrepassDescriptor);

    indicesBuffer = BufferArray<uint32_t>(0, { vg::BufferUsage::IndexBuffer });
    verticesBuffer = BufferArray<glm::vec3>(0, { vg::BufferUsage::VertexBuffer });
    normalsBuffer = BufferArray<glm::vec3>(0, { vg::BufferUsage::VertexBuffer });
    uvsBuffer = BufferArray<glm::vec2>(0, { vg::BufferUsage::VertexBuffer });
    meshBoundingBoxes = BufferArray<BoundingBox>(0, { vg::BufferUsage::StorageBuffer });

    batchMaterial = BufferArray<uint32_t>(0, vg::BufferUsage::StorageBuffer);
    objectsBuffer = BufferArray<RenderObject>(0, { vg::BufferUsage::StorageBuffer });
    instanceBuffer = BufferArray<glm::mat4>(0, { vg::BufferUsage::StorageBuffer });
    mappingBuffer = BufferArray<uint32_t>(0, { vg::BufferUsage::StorageBuffer, vg::BufferUsage::VertexBuffer });
    instructionsBuffer = BufferArray<vg::cmd::DrawIndexed>(0, { vg::BufferUsage::StorageBuffer, vg::BufferUsage::IndirectBuffer });
    depthPyramid = DepthPyramid(depthImage.GetDimensions()[0], depthImage.GetDimensions()[1]);
    cameraMatrices = BufferArray<glm::mat4>(3, { vg::BufferUsage::UniformBuffer });
    depthPrepassFramebuffer = vg::Framebuffer(depthPrepass, { depthView }, depthImage.GetDimensions()[0], depthImage.GetDimensions()[1]);
    query = vg::QueryPool(vg::QueryType::Timestamp, 12);

    renderDescriptor.AttachBuffer(vg::DescriptorType::StorageBuffer, materials[0].variants, 0, materials[0].variants.byte_size(), 4, 0);
}

GPUDrivenRendererSystem::GPUDrivenRendererSystem(GPUDrivenRendererSystem&& rhs) noexcept
    :GPUDrivenRendererSystem()
{
    *this = std::move(rhs);
}

GPUDrivenRendererSystem& GPUDrivenRendererSystem::operator=(GPUDrivenRendererSystem&& rhs)
{
    objectCount++;
    if (this != &rhs)
    {
        std::swap(descriptorPool, rhs.descriptorPool);
        std::swap(indicesBuffer, rhs.indicesBuffer);
        std::swap(verticesBuffer, rhs.verticesBuffer);
        std::swap(normalsBuffer, rhs.normalsBuffer);
        std::swap(uvsBuffer, rhs.uvsBuffer);
        std::swap(meshReferances, rhs.meshReferances);
        std::swap(meshBoundingBoxes, rhs.meshBoundingBoxes);
        std::swap(batchMap, rhs.batchMap);
        std::swap(batches, rhs.batches);
        std::swap(objectsBuffer, rhs.objectsBuffer);
        std::swap(instanceBuffer, rhs.instanceBuffer);
        std::swap(mappingBuffer, rhs.mappingBuffer);
        std::swap(instructionsBuffer, rhs.instructionsBuffer);
        std::swap(depthPyramid, rhs.depthPyramid);
        std::swap(cameraMatrices, rhs.cameraMatrices);
        std::swap(depthPrepassFramebuffer, rhs.depthPrepassFramebuffer);
        std::swap(computeDescriptor, rhs.computeDescriptor);
        std::swap(renderDescriptor, rhs.renderDescriptor);
        std::swap(instructionDescriptors, rhs.instructionDescriptors);
        std::swap(query, rhs.query);
        std::swap(dataChanged, rhs.dataChanged);
        std::swap(batchMaterial, rhs.batchMaterial);
        std::swap(depthPrepassDescriptor, rhs.depthPrepassDescriptor);
    }

    return *this;
}

GPUDrivenRendererSystem::~GPUDrivenRendererSystem()
{
    objectCount--;
    if (objectCount == 0)
    {
        renderPass.~RenderPass();
        depthPrepass.~RenderPass();
        computeRenderer.~ComputePipeline();
        clearInstructions.~ComputePipeline();
    }
}

void GPUDrivenRendererSystem::CopyData(GPUDrivenRendererSystem& rendererSystem)
{
    if (rendererSystem.dataChanged != BufferUpdate::None)
    {
        if (rendererSystem.dataChanged.IsSet(BufferUpdate::Indices))
            indicesBuffer = rendererSystem.indicesBuffer;
        if (rendererSystem.dataChanged.IsSet(BufferUpdate::Vertices))
            verticesBuffer = rendererSystem.verticesBuffer;
        if (rendererSystem.dataChanged.IsSet(BufferUpdate::Normals))
            normalsBuffer = rendererSystem.normalsBuffer;
        if (rendererSystem.dataChanged.IsSet(BufferUpdate::Uvs))
            uvsBuffer = rendererSystem.uvsBuffer;
        if (rendererSystem.dataChanged.IsSet(BufferUpdate::Referances))
            meshReferances = rendererSystem.meshReferances;
        if (rendererSystem.dataChanged.IsSet(BufferUpdate::BoundingBoxes))
            meshBoundingBoxes = rendererSystem.meshBoundingBoxes;

        if (rendererSystem.dataChanged.IsSet(BufferUpdate::RenderObjects))
        {
            batchMaterial = rendererSystem.batchMaterial;
            batchMap = rendererSystem.batchMap;
            batches = rendererSystem.batches;
            objectsBuffer = rendererSystem.objectsBuffer;
            batchMap = rendererSystem.batchMap;
            instanceBuffer = rendererSystem.instanceBuffer;
            mappingBuffer = rendererSystem.mappingBuffer;
        }

        if (rendererSystem.dataChanged.IsSet(BufferUpdate::RenderObjects) || rendererSystem.dataChanged.IsSet(BufferUpdate::Referances))
            instructionsBuffer = rendererSystem.instructionsBuffer;
        rendererSystem.dataChanged = 0;
    }
}

uint32_t GPUDrivenRendererSystem::AddMesh(Span<const uint32_t> triangles, Span<const glm::vec3> vertices, Span<const glm::vec3> normals, Span<const glm::vec2> uvs)
{
    dataChanged.Set(BufferUpdate::Mesh);
    BoundingBox boundingBox;
    for (const glm::vec3& vertex : vertices)
    {
        if (vertex.x < boundingBox.min.x)
            boundingBox.min.x = vertex.x;
        else if (vertex.x > boundingBox.max.x)
            boundingBox.max.x = vertex.x;

        if (vertex.y < boundingBox.min.y)
            boundingBox.min.y = vertex.y;
        else if (vertex.y > boundingBox.max.y)
            boundingBox.max.y = vertex.y;

        if (vertex.z < boundingBox.min.z)
            boundingBox.min.z = vertex.z;
        else if (vertex.z > boundingBox.max.z)
            boundingBox.max.z = vertex.z;
    }
    meshReferances.push_back(MeshReferance(verticesBuffer.size(), indicesBuffer.size(), triangles.size()));
    meshBoundingBoxes.push_back(boundingBox);

    indicesBuffer.insert_range(triangles, indicesBuffer.size());
    verticesBuffer.insert_range(vertices, verticesBuffer.size());
    normalsBuffer.insert_range(normals, normalsBuffer.size());
    uvsBuffer.insert_range(uvs, uvsBuffer.size());

    return meshReferances.size() - 1;
}

std::span<glm::vec3> GPUDrivenRendererSystem::GetMeshVertices(uint32_t id)
{
    dataChanged.Set(BufferUpdate::Vertices);
    const MeshReferance& ref = meshReferances[id];
    return std::span<glm::vec3>(verticesBuffer.begin() + ref.vertexStart, verticesBuffer.end() + ref.triangleCount / 3);
}

void GPUDrivenRendererSystem::ReserveRenderObjects(int count)
{
    dataChanged.Set(BufferUpdate::RenderObjects);
    objectsBuffer.reserve(count);
    instanceBuffer.reserve(count);
}

void GPUDrivenRendererSystem::AddRenderObject(uint32_t mesh, uint32_t material, uint32_t variant, const glm::mat4& matrix)
{
    dataChanged.Set(BufferUpdate::RenderObjects);
    uint32_t objectID = objectsBuffer.size();
    uint32_t batchID = 0;
    int ID = (material << 24) | ((variant << 16)) | mesh;
    if (batchMap.contains(ID))
    {
        batchID = batchMap[ID];
        batches[batchID].objectCount++;
        batchMaterial[batchID] = (material << 8) | variant;
    }
    else
    {
        batchID = batchMap.size();
        batches.push_back(Batch(mesh, 1));
        batchMaterial.push_back((material << 8) | variant);
        batchMap[ID] = batchID;
    }

    objectsBuffer.push_back(RenderObject(objectID, batchID, mesh));
    instanceBuffer.push_back(matrix);
}
void GPUDrivenRendererSystem::RemoveRenderObject(uint32_t id)
{
    dataChanged.Set(BufferUpdate::RenderObjects);
    uint32_t batchID = objectsBuffer[id].batchID;
    objectsBuffer.erase(id);
    instanceBuffer.erase(id);
    batches[batchID].objectCount--;
    if (batches[batchID].objectCount == 0)
    {
        batches.erase(batches.begin() + batchID);
        batchMaterial.erase(batchID);
        batchMap.erase(batchID);
    }
}

void GPUDrivenRendererSystem::GetRenderCommands(const vg::Image& depthImage, const vg::ImageView& depthView, vg::CmdBuffer& buffer, const glm::mat4& cameraView, const glm::mat4& cameraProjection, const vg::Framebuffer& framebuffer, uint32_t width, uint32_t height)
{
    if (objectsBuffer.size() == 0)
        return;

    if (dataChanged.IsSet(BufferUpdate::RenderObjects))
    {
        mappingBuffer.resize(objectsBuffer.size());
        instructionsBuffer.resize(batches.size());
        uint32_t offset = 0;
        for (int j = 0; j < batches.size(); j++)
        {
            auto& ref = meshReferances[batches[j].meshID];
            instructionsBuffer[j] = vg::cmd::DrawIndexed(ref.triangleCount, 0, ref.triangleStart, ref.vertexStart, offset);
            offset += batches[j].objectCount;
        }
    }

    depthPrepassDescriptor.AttachBuffer(vg::DescriptorType::UniformBuffer, cameraMatrices, 0, cameraMatrices.byte_size(), 0, 0);
    depthPrepassDescriptor.AttachBuffer(vg::DescriptorType::StorageBuffer, instanceBuffer, 0, instanceBuffer.byte_size(), 1, 0);
    renderDescriptor.AttachBuffer(vg::DescriptorType::UniformBuffer, cameraMatrices, 0, cameraMatrices.byte_size(), 0, 0);
    renderDescriptor.AttachBuffer(vg::DescriptorType::StorageBuffer, instanceBuffer, 0, instanceBuffer.byte_size(), 1, 0);
    renderDescriptor.AttachBuffer(vg::DescriptorType::StorageBuffer, objectsBuffer, 0, objectsBuffer.byte_size(), 2, 0);
    renderDescriptor.AttachBuffer(vg::DescriptorType::StorageBuffer, batchMaterial, 0, batchMaterial.byte_size(), 3, 0);
    for (int i = 0; i < ECS::System<Transform>::components.size(); i++)
    {
        auto& mesh = ECS::System<Transform>::components[i].GetComponent<MeshArray>();
        instanceBuffer[&mesh - &components[0]] = ECS::System<Transform>::components[i].Matrix();
    }

    cameraMatrices[0] = cameraProjection;
    cameraMatrices[1] = cameraView;
    cameraMatrices[2] = cameraProjection * cameraView;


    buffer.Append(
        vg::cmd::BeginRenderpass(depthPrepass, depthPrepassFramebuffer, { 0, 0 }, { width, height }, { vg::ClearDepthStencil{1.0f,0U} }, vg::SubpassContents::Inline),
        vg::cmd::BindPipeline(depthPrepass.GetPipelines()[0]),
        vg::cmd::BindDescriptorSets(depthPrepass.GetPipelineLayouts()[0], vg::PipelineBindPoint::Graphics, 0, { depthPrepassDescriptor }),
        vg::cmd::BindVertexBuffers(std::vector<vg::BufferHandle>{ verticesBuffer, normalsBuffer, uvsBuffer, mappingBuffer }, { 0, 0, 0, 0 }, 0),
        vg::cmd::BindIndexBuffer(indicesBuffer, 0, vg::IndexType::Uint32),
        vg::cmd::SetViewport(vg::Viewport(width, height)),
        vg::cmd::SetScissor(vg::Scissor(width, height)),
        vg::cmd::DrawIndexedIndirect(instructionsBuffer, 0, batches.size(), sizeof(vg::cmd::DrawIndexed)),
        vg::cmd::EndRenderpass(),
        depthPyramid.Generate(depthImage, depthView)
    );

    computeDescriptor.AttachBuffer(vg::DescriptorType::UniformBuffer, cameraMatrices, 0, cameraMatrices.byte_size(), 0, 0);
    computeDescriptor.AttachBuffer(vg::DescriptorType::StorageBuffer, objectsBuffer, 0, objectsBuffer.byte_size(), 1, 0);
    computeDescriptor.AttachBuffer(vg::DescriptorType::StorageBuffer, instructionsBuffer, 0, batches.size() * sizeof(vg::cmd::DrawIndexed), 2, 0);
    computeDescriptor.AttachBuffer(vg::DescriptorType::StorageBuffer, mappingBuffer, 0, mappingBuffer.byte_size(), 3, 0);
    computeDescriptor.AttachBuffer(vg::DescriptorType::StorageBuffer, meshBoundingBoxes, 0, meshBoundingBoxes.byte_size(), 4, 0);
    computeDescriptor.AttachBuffer(vg::DescriptorType::StorageBuffer, instanceBuffer, 0, instanceBuffer.byte_size(), 5, 0);
    computeDescriptor.AttachImage(vg::DescriptorType::CombinedImageSampler, vg::ImageLayout::ShaderReadOnlyOptimal, depthPyramid.pyramidImageView, depthPyramid.reductionSampler, 6, 0);
    instructionDescriptors.AttachBuffer(vg::DescriptorType::StorageBuffer, instructionsBuffer, 0, batches.size() * sizeof(vg::cmd::DrawIndexed), 0, 0);

    buffer.Append(
        vg::cmd::BindPipeline(clearInstructions),
        vg::cmd::PushConstants(clearInstructions.GetPipelineLayout(), vg::ShaderStage::Compute, 0, instructionsBuffer.size()),
        vg::cmd::BindDescriptorSets(clearInstructions.GetPipelineLayout(), vg::PipelineBindPoint::Compute, 0, { instructionDescriptors }),
        vg::cmd::Dispatch(ceil(instructionsBuffer.size() / 32.0f), 1, 1),
        vg::cmd::PipelineBarier(vg::PipelineStage::ComputeShader, vg::PipelineStage::ComputeShader, { vg::BufferMemoryBarrier(vg::Access::ShaderWrite, vg::Access::ShaderRead, instructionsBuffer, 0, instructionsBuffer.byte_size()) }),
        vg::cmd::BindPipeline(computeRenderer),
        vg::cmd::PushConstants(computeRenderer.GetPipelineLayout(), vg::ShaderStage::Compute, 0, std::make_tuple(depthPyramid.pyramidImage.GetDimensions()[1], depthPyramid.pyramidImage.GetDimensions()[0], instanceBuffer.size())),
        vg::cmd::BindDescriptorSets(computeRenderer.GetPipelineLayout(), vg::PipelineBindPoint::Compute, 0, { computeDescriptor }),
        vg::cmd::Dispatch(ceil(instanceBuffer.size() / 1024.0f), 1, 1),
        vg::cmd::PipelineBarier(vg::PipelineStage::ComputeShader, vg::PipelineStage::DrawIndirect, { vg::MemoryBarrier(vg::Access::ShaderWrite, vg::Access::IndirectCommandRead) })
    );

    buffer.Append(
        vg::cmd::BeginRenderpass(renderPass, framebuffer, { 0, 0 }, { width, height }, { vg::ClearColor{ 0.4f * 0.5,0.95f * 0.5,1.0f * 0.5,1.0f },vg::ClearDepthStencil{1.0f,0U} }, vg::SubpassContents::Inline),
        vg::cmd::BindPipeline(renderPass.GetPipelines()[0]),
        vg::cmd::BindDescriptorSets(renderPass.GetPipelineLayouts()[0], vg::PipelineBindPoint::Graphics, 0, { renderDescriptor }),
        vg::cmd::BindVertexBuffers(std::vector<vg::BufferHandle>{ verticesBuffer, normalsBuffer, uvsBuffer, mappingBuffer }, { 0, 0, 0, 0 }, 0),
        vg::cmd::BindIndexBuffer(indicesBuffer, 0, vg::IndexType::Uint32),
        vg::cmd::SetViewport(vg::Viewport(width, height)),
        vg::cmd::SetScissor(vg::Scissor(width, height)),
        vg::cmd::DrawIndexedIndirect(instructionsBuffer, 0, batches.size(), sizeof(vg::cmd::DrawIndexed))
    );
}