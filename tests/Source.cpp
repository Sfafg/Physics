#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <math.h>
#include "VG/VG.h"
#include "DebugDraw.h"
#include "Physics.h"

using namespace ECS;
using namespace vg;
bool recreateFramebuffer = false;
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_CURSOR_HIDDEN, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Marching Cubes", nullptr, nullptr);
    // glfwSetWindowPos(window, -1920, 0);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int w, int h) {recreateFramebuffer = true; });
    int w, h; glfwGetFramebufferSize(window, &w, &h);

    vg::instance = Instance({ "VK_KHR_surface", "VK_KHR_win32_surface" },
        [](MessageSeverity severity, const char* message) {
            if (severity < MessageSeverity::Warning) return;
            std::cout << message << '\n' << '\n';
        });

    SurfaceHandle windowSurface = Window::CreateWindowSurface(vg::instance, window);
    DeviceFeatures deviceFeatures({ Feature::LogicOp,Feature::SampleRateShading, Feature::FillModeNonSolid });
    Queue generalQueue({ QueueType::General }, 1.0f);
    Device rendererDevice({ &generalQueue }, { "VK_KHR_swapchain" }, deviceFeatures, windowSurface,
        [](auto id, auto supportedQueues, auto supportedExtensions, auto type, DeviceLimits limits, DeviceFeatures features) {
            return (type == DeviceType::Dedicated);
        });
    vg::currentDevice = &rendererDevice;

    Shader fragmentShader(ShaderStage::Fragment, "resources/shaders/shader.frag.spv");
    Shader vertexShader(ShaderStage::Vertex, "resources/shaders/shader.vert.spv");
    Renderer::materials.push_back(
        Material(
            Subpass(
                GraphicsPipeline(
                    {
                        {
                            { 0, DescriptorType::UniformBuffer, 1, {ShaderStage::Vertex,ShaderStage::Fragment} }, // Camera Uniform
                        },
                        {
                            { 0, DescriptorType::UniformBuffer, 1, {ShaderStage::Vertex,ShaderStage::Fragment} } // Model Uniform
                        }
                    },
                    {},
                    std::vector<Shader*>{ &vertexShader, & fragmentShader },
                    VertexLayout({ vg::VertexBinding(0, sizeof(glm::vec3)),vg::VertexBinding(1, sizeof(glm::vec3)) }, { vg::VertexAttribute(0, 0, vg::Format::RGB32SFLOAT),vg::VertexAttribute(1, 1, vg::Format::RGB32SFLOAT) }),
                    InputAssembly(Primitive::TriangleStrip, true),
                    Tesselation(),
                    ViewportState(Viewport(w, h), Scissor(w, h)),
                    Rasterizer(false, PolygonMode::Line, CullMode::Back),
                    Multisampling(1, true),
                    DepthStencil(true, true, CompareOp::Less),
                    ColorBlending(true, LogicOp::Copy, { 0,0,0,0 }, { ColorBlend() }),
                    { DynamicState::Viewport, DynamicState::Scissor },
                    1
                ),
                {}, { AttachmentReference(0, ImageLayout::ColorAttachmentOptimal) },
                {}, AttachmentReference(1, ImageLayout::DepthStencilAttachmentOptimal)
            ),
            SubpassDependency(-1, 0, PipelineStage::ColorAttachmentOutput, PipelineStage::ColorAttachmentOutput, 0, Access::ColorAttachmentWrite, {})
        )
    );

    Shader gizmoVert(ShaderStage::Vertex, "resources/shaders/gizmo.vert.spv");
    Shader gizmoFrag(ShaderStage::Fragment, "resources/shaders/gizmo.frag.spv");
    Renderer::materials.push_back(
        Material(
            Subpass(
                GraphicsPipeline(
                    0U,
                    std::nullopt, std::nullopt,
                    std::vector<Shader*>{ &gizmoVert, & gizmoFrag },
                    std::nullopt,
                    InputAssembly(Primitive::TriangleStrip, true),
                    std::nullopt,
                    std::nullopt,
                    Rasterizer(false, PolygonMode::Fill, CullMode::Back)
                ),
                {}, { AttachmentReference(0, ImageLayout::ColorAttachmentOptimal) },
                {}, AttachmentReference(1, ImageLayout::DepthStencilAttachmentOptimal)
            ),
            SubpassDependency(0, 1, PipelineStage::ColorAttachmentOutput, PipelineStage::TopOfPipe, Access::ColorAttachmentWrite, 0, {})
        )
    );

    Renderer::Init(windowSurface, w, h, generalQueue);
    DebugDraw::Init(generalQueue);

    std::vector<glm::vec3> pos;
    std::vector<glm::vec3> norm;
    std::vector<uint16_t> index;

    const int resolution = 32;
    float radius = 1;
    glm::vec3 color = { 1,1,1 };
    glm::vec3 position = { 0.0f,0.0f,0.0f };
    const float PI = 3.14159265359f;
    for (int inclination = 0; inclination <= resolution; inclination++)
    {
        for (int azimuth = 0; azimuth < resolution * 2; azimuth++)
        {
            float a = inclination / (float) resolution * PI;
            float b = azimuth / (float) resolution * PI;

            glm::vec3 p;
            p.x = radius * sin(a) * cos(b);
            p.y = radius * sin(a) * sin(b);
            p.z = radius * cos(a);
            pos.push_back(position + p);
            norm.push_back(glm::normalize(p));
        }
    }

    for (int i = 0; i < resolution; i++)
    {
        for (int j = 0; j < resolution * 2; j++)
        {
            index.push_back(i * resolution * 2 + j);
            index.push_back((i + 1) * resolution * 2 + j);
        }
        index.push_back(i * resolution * 2);
        index.push_back((i + 1) * resolution * 2);
        index.push_back(-1);
    }

    BufferArray positions(pos, vg::BufferUsage::VertexBuffer, vg::MemoryProperty::HostVisible);
    BufferArray normals(norm, vg::BufferUsage::VertexBuffer, vg::MemoryProperty::HostVisible);
    BufferArray indices(index, vg::BufferUsage::IndexBuffer, vg::MemoryProperty::HostVisible);
    Mesh sphereMesh({ positions,normals }, indices, IndexType::Uint16, index.size(), 0);

    pos.assign({
        {-1,-1,-1},
        {-1, 1,-1},
        { 1,-1,-1},
        { 1, 1,-1},
        {-1,-1, 1},
        {-1, 1, 1},
        { 1,-1, 1},
        { 1, 1, 1}
        });
    norm.assign({
         glm::normalize(pos[0]),
        glm::normalize(pos[0]),
        glm::normalize(pos[2]),
        glm::normalize(pos[3]),
        glm::normalize(pos[4]),
        glm::normalize(pos[5]),
        glm::normalize(pos[6]),
        glm::normalize(pos[7])
        });

    index.assign({ 0,1,2,3,7,1,5,0,4,2,6,7,4,5 });
    positions = BufferArray(pos, vg::BufferUsage::VertexBuffer, vg::MemoryProperty::HostVisible);
    normals = BufferArray(norm, vg::BufferUsage::VertexBuffer, vg::MemoryProperty::HostVisible);
    indices = BufferArray(index, vg::BufferUsage::IndexBuffer, vg::MemoryProperty::HostVisible);
    Mesh cubeMesh({ positions,normals }, indices, IndexType::Uint16, index.size(), 0);

    Entity rigidBodies[250];
    rigidBodies[0] = Entity::AddEntity(
        Transform({ 0,0,-0.25 }, { 10,10,0.5 }),
        Collider({ 10,10,0.5 }),
        RigidBody({ 0,0,0 }, { 0,0,0 }, { 0,0,0 }, std::numeric_limits<double>::infinity(), glm::mat3(std::numeric_limits<double>::infinity()), 0.4),
        MeshArray({ cubeMesh })
    );
    // rigidBodies[1] = Entity::AddEntity(
    //     Transform({ 10.25,0,10.26 }, { 0.5,10,10 }),
    //     Collider({ 0.5,10,10 }),
    //     RigidBody({ 0,0,0 }, { 0,0,0 }, { 0,0,0 }, std::numeric_limits<double>::infinity(), glm::mat3(std::numeric_limits<double>::infinity())),
    //     MeshArray({ cubeMesh })
    // );
    // rigidBodies[2] = Entity::AddEntity(
    //     Transform({ -10.25,0,10.26 }, { 0.5,10,10 }),
    //     Collider({ 0.5,10,10 }),
    //     RigidBody({ 0,0,0 }, { 0,0,0 }, { 0,0,0 }, std::numeric_limits<double>::infinity(), glm::dmat3(std::numeric_limits<double>::infinity())),
    //     MeshArray({ cubeMesh })
    // );
    // rigidBodies[3] = Entity::AddEntity(
    //     Transform({ 0,10.25,10.26 }, { 10,0.5,10 }),
    //     Collider({ 10,0.5,10 }),
    //     RigidBody({ 0,0,0 }, { 0,0,0 }, { 0,0,0 }, std::numeric_limits<double>::infinity(), glm::dmat3(std::numeric_limits<double>::infinity())),
    //     MeshArray({ cubeMesh })
    // );
    // rigidBodies[4] = Entity::AddEntity(
    //     Transform({ 0,-10.25,10.26 }, { 10,0.5,10 }),
    //     Collider({ 10,0.5,10 }),
    //     RigidBody({ 0,0,0 }, { 0,0,0 }, { 0,0,0 }, std::numeric_limits<double>::infinity(), glm::dmat3(std::numeric_limits<double>::infinity())),
    //     MeshArray({ cubeMesh })
    // );

    // rigidBodies[5] = Entity::AddEntity(
    //     Transform({ 0, 0, 3.5 }, { 2,2,1 }),
    //     Collider({ 2,2,1 }),
    //     RigidBody({ 8,0,10 }, { 0,0,0 }, { 0,0,0 }, 4.0 * 4.0 * 2.0, glm::dmat3(1.0 / 12.0 * (4 * 4 + 2 * 2), 0, 0, 0, 1.0 / 12.0 * (4 * 4 + 2 * 2), 0, 0, 0, 1.0 / 12.0 * (4 * 4 + 4 * 4)), 0.35),
    //     MeshArray({ cubeMesh })
    // );
    rigidBodies[1] = Entity::AddEntity(
        Transform({ 0, 0, 0 }, { 1,1,1 }),
        Collider({ 1,1,1 }),
        RigidBody({ 0,0,0 }, { 0,0,0 }, { 0,0,0 }, 1, glm::dmat3(1.0 / 6.0 * 2 * 2), 0.0, 0.2, 0.3),
        MeshArray({ cubeMesh })
    );

    Transform cameraTransform({ 0.5,-2.5,0.5 });
    static float fov = 70;
    glfwSetScrollCallback(window, [](GLFWwindow* window, double scrollx, double scrolly)
        {
            fov = fov - pow(scrolly, 3);
            if (fov < 0) fov = 0;
            if (fov > 180) fov = 180;
        });
    glm::dvec2 lastMousePos;
    glfwGetCursorPos(window, &lastMousePos.x, &lastMousePos.y);
    bool enterPressed = false;
    int frameCount = 0;
    int rbs = 6;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, true);

        {
            glm::dvec2 mousePos;
            glfwGetCursorPos(window, &mousePos.x, &mousePos.y);
            auto mouseDelta = (lastMousePos - mousePos) * 0.00006 * (double) fov;
            lastMousePos = mousePos;

            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
            {
                auto rotation = glm::rotate(cameraTransform.rotation, mouseDelta.x, { 0,0,1 });
                cameraTransform.rotation = glm::rotate(rotation, mouseDelta.y, { 1,0,0 });
            }

            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
            {
                auto rotation = glm::rotate(rigidBodies[0].GetComponent<Transform>().rotation, mouseDelta.x, { 0,0,1 });
                rigidBodies[0].GetComponent<Transform>().rotation = glm::rotate(rotation, mouseDelta.y, { 1,0,0 });
            }

            double speed = 0.01;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
                speed *= 0.2;
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
                speed *= 3;
            if (glfwGetKey(window, GLFW_KEY_W))
                cameraTransform.position += cameraTransform.Forward() * speed;

            if (glfwGetKey(window, GLFW_KEY_S))
                cameraTransform.position -= cameraTransform.Forward() * speed;

            if (glfwGetKey(window, GLFW_KEY_D))
                cameraTransform.position += cameraTransform.Right() * speed;

            if (glfwGetKey(window, GLFW_KEY_A))
                cameraTransform.position -= cameraTransform.Right() * speed;

            if (glfwGetKey(window, GLFW_KEY_E))
                cameraTransform.position += cameraTransform.Up() * speed;

            if (glfwGetKey(window, GLFW_KEY_Q))
                cameraTransform.position -= cameraTransform.Up() * speed;

            if (glfwGetKey(window, GLFW_KEY_UP))
                rigidBodies[0].GetComponent<Transform>().position += glm::dvec3{ 0,0,1 } *speed;

            if (glfwGetKey(window, GLFW_KEY_DOWN))
                rigidBodies[0].GetComponent<Transform>().position -= glm::dvec3{ 0,0,1 } *speed;

            if (glfwGetKey(window, GLFW_KEY_RIGHT))
                rigidBodies[0].GetComponent<Transform>().position += glm::dvec3{ 1,0,0 } *speed;

            if (glfwGetKey(window, GLFW_KEY_LEFT))
                rigidBodies[0].GetComponent<Transform>().position -= glm::dvec3{ 1,0,0 } *speed;

            if (glfwGetKey(window, GLFW_KEY_KP_8))
                rigidBodies[0].GetComponent<Transform>().position += glm::dvec3{ 0,1,0 } *speed;

            if (glfwGetKey(window, GLFW_KEY_KP_2))
                rigidBodies[0].GetComponent<Transform>().position -= glm::dvec3{ 0,1,0 } *speed;
        }

        auto p = ColliderManager::GetPenetrations();
        if (glfwGetKey(window, GLFW_KEY_ENTER))
        {
            if (!enterPressed || !glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || true)
            {
                for (int k = 0; k < 4; k++)
                {
                    Physics::Update(1 / 64.0, 16);
                    if (frameCount % 100 == 99 && rbs != 30)
                    {
                        int type = rand() % 2;
                        dvec3 pos;
                        pos.x = rand() / double(RAND_MAX) * 7 - 3.5;
                        pos.y = rand() / double(RAND_MAX) * 7 - 3.5;
                        pos.z = 25;
                        dvec3 scale;
                        scale.x = rand() / double(RAND_MAX) + 1.75;
                        scale.y = rand() / double(RAND_MAX) + 1.75;
                        scale.z = rand() / double(RAND_MAX) + 1.75;
                        dvec3 velocity;
                        velocity.x = rand() / double(RAND_MAX) * 7 - 3.5;
                        velocity.y = rand() / double(RAND_MAX) * 7 - 3.5;
                        velocity.z = rand() / double(RAND_MAX) * 7 - 3.5;
                        if (type == 0)
                        {
                            double mass = 4.0 / 3.0 * glm::pi<double>() * scale.x * scale.x * 0.2;
                            rigidBodies[rbs] = Entity::AddEntity(
                                Transform(pos, { scale.x,scale.x,scale.x }),
                                Collider({ scale.x }),
                                RigidBody(velocity, { 0,0,0 }, { 0,0,0 }, mass, glm::dmat3(2.0 / 5.0), 0.35),
                                MeshArray({ sphereMesh })
                            );
                        }
                        else
                        {
                            double mass = scale.x * 2.0 * scale.y * 2.0 * scale.z * 2.0;
                            rigidBodies[rbs] = Entity::AddEntity(
                                Transform(pos, scale),
                                Collider(scale),
                                RigidBody(velocity, { 0,0,0 }, { 0,0,0 }, mass, glm::dmat3(1.0 / 12.0 * (pow(scale.x * 2.0, 2.0) + pow(scale.z * 2.0, 2.0)), 0, 0, 0, 1.0 / 12.0 * (pow(scale.y * 2.0, 2.0) + pow(scale.z * 2.0, 2.0)), 0, 0, 0, 1.0 / 12.0 * (pow(scale.x * 2.0, 2.0) + pow(scale.y * 2.0, 2.0))), 0.35),
                                MeshArray({ cubeMesh })
                            );
                        }

                        rbs++;
                    }
                    frameCount++;
                }
                // std::cout << frameCount * 1 << "\n";
            }
            enterPressed = true;
        }
        else enterPressed = false;

        if (recreateFramebuffer)
            glfwGetFramebufferSize(window, &w, &h);

        Renderer::Run(cameraTransform, fov, generalQueue, recreateFramebuffer, w, h);
        recreateFramebuffer = false;
    }
    DebugDraw::Free();
    Renderer::Free();
    glfwTerminate();
}