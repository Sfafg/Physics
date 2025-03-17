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
#include "Physics.h"
#include "Renderer.h"
#include "ObjLoader.h"
#include "UISystems.h"
using namespace std::chrono_literals;
using namespace ECS;
using namespace vg;

std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec2>, std::vector<uint32_t>> GenerateSphereMesh()
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<uint32_t> indices;

    const int resolution = 14;
    for (int inclination = 0; inclination <= resolution; inclination++)
    {
        for (int azimuth = 0; azimuth < resolution * 2; azimuth++)
        {
            float a = inclination / (float) resolution * glm::pi<float>();
            float b = azimuth / (float) resolution * glm::pi<float>();

            glm::vec3 p;
            p.x = sin(a) * cos(b);
            p.y = sin(a) * sin(b);
            p.z = cos(a);
            vertices.push_back(p);
            normals.push_back(p);
            uvs.push_back({ 0,0 });
        }
    }

    for (int i = 0; i < resolution; i++)
    {
        for (int j = 0; j < resolution * 2; j++)
        {
            indices.push_back(i * resolution * 2 + j);
            indices.push_back(i * resolution * 2 + j + 1);
            indices.push_back((i + 1) * resolution * 2 + j);

            indices.push_back(i * resolution * 2 + j + 1);
            indices.push_back((i + 1) * resolution * 2 + j + 1);
            indices.push_back((i + 1) * resolution * 2 + j);
        }
    }

    return { vertices, normals,uvs,indices };
}
bool drawUI = false;
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_CURSOR_HIDDEN, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Marching Cubes", nullptr, nullptr);
    // glfwSetWindowPos(window, -1920, 0);
    int w, h; glfwGetFramebufferSize(window, &w, &h);

    vg::instance = Instance({ "VK_KHR_surface", "VK_KHR_win32_surface" },
        [](MessageSeverity severity, const char* message) {
            if (severity < MessageSeverity::Warning);
            std::cout << message << '\n' << '\n';
        }, false);

    SurfaceHandle windowSurface = Window::CreateWindowSurface(vg::instance, window);
    DeviceFeatures deviceFeatures({ Feature::LogicOp,Feature::SampleRateShading, Feature::FillModeNonSolid, Feature::MultiDrawIndirect });
    Queue generalQueue({ QueueType::General }, 1.0f);
    Device rendererDevice({ &generalQueue }, { "VK_KHR_swapchain","VK_EXT_sampler_filter_minmax" }, deviceFeatures, windowSurface,
        [](auto id, auto supportedQueues, auto supportedExtensions, auto type, DeviceLimits limits, DeviceFeatures features) {
            return (type == DeviceType::Dedicated);
        });
    vg::currentDevice = &rendererDevice;

    Shader fragmentShader(ShaderStage::Fragment, "resources/shaders/shader.frag.spv");
    Shader vertexShader(ShaderStage::Vertex, "resources/shaders/shader.vert.spv");
    Renderer::materials.emplace_back(
        Material(
            std::vector<vg::Shader*>{ &vertexShader, & fragmentShader },
            vg::InputAssembly(vg::Primitive::Triangles, false),
            vg::Rasterizer(false, false, vg::PolygonMode::Fill, vg::CullMode::Back, vg::FrontFace::Clockwise),
            vg::Multisampling(1, false),
            vg::DepthStencil(true, true, vg::CompareOp::Less),
            vg::ColorBlending(true, vg::LogicOp::Copy, { 0,0,0,0 }, { vg::ColorBlend() }),
            { vg::DynamicState::Viewport, vg::DynamicState::Scissor }
        ));

    Renderer::materials[0].AddVariant(VariantData{ 1.0f, 0.45f, 0.0f });
    Renderer::materials[0].AddVariant(VariantData{ 0.0f, 1.0f, 0.2f });
    Renderer::materials[0].AddVariant(VariantData{ 0.0f, 0.0f, 1.0f });

    Renderer::materials[0].AddVariant(VariantData{ 0.8f, 0.9f, 1.0f });
    Renderer::materials[0].AddVariant(VariantData{ 1.0f, 0.2f, 0.1f });
    Renderer::materials[0].AddVariant(VariantData{ 0.3f, 1.0f, 0.5f });
    Renderer::materials[0].AddVariant(VariantData{ 0.4f, 0.3f, 0.2f });
    Renderer::materials[0].AddVariant(VariantData{ 0.3f, 0.25f, 0.22f });
    Renderer::materials[0].AddVariant(VariantData{ 1.0f, 1.0f, 0.0f });

    Renderer::Init(window, windowSurface, w, h);
    {
        {
            auto [vertices, normals, uvs, triangles] = GenerateSphereMesh();
            Renderer::renderSystem[0].AddMesh(triangles, vertices, normals, uvs);
        }
        {
            auto [vertices, normals, triangles] = LoadMesh("resources/Box.obj");
            std::vector<glm::vec2> uvs(vertices.size());
            Renderer::renderSystem[0].AddMesh(triangles, vertices, normals, uvs);
        }
        {
            auto [vertices, normals, triangles] = LoadMesh("resources/Kieliszek.obj");
            std::vector<glm::vec2> uvs(vertices.size());
            Renderer::renderSystem[0].AddMesh(triangles, vertices, normals, uvs);
        }
        {
            auto [vertices, normals, triangles] = LoadMesh("resources/Parasol.obj");
            std::vector<glm::vec2> uvs(vertices.size());
            Renderer::renderSystem[0].AddMesh(triangles, vertices, normals, uvs);
        }
        {
            auto [vertices, normals, triangles] = LoadMesh("resources/Parasolka.obj");
            std::vector<glm::vec2> uvs(vertices.size());
            Renderer::renderSystem[0].AddMesh(triangles, vertices, normals, uvs);
        }
        {
            auto [vertices, normals, triangles] = LoadMesh("resources/Stolek.obj");
            std::vector<glm::vec2> uvs(vertices.size());
            Renderer::renderSystem[0].AddMesh(triangles, vertices, normals, uvs);
        }
        {
            auto [vertices, normals, triangles] = LoadMesh("resources/Stolik.obj");
            std::vector<glm::vec2> uvs(vertices.size());
            Renderer::renderSystem[0].AddMesh(triangles, vertices, normals, uvs);
        }
        {
            auto [vertices, normals, triangles] = LoadMesh("resources/Ziemia.obj");
            std::vector<glm::vec2> uvs(vertices.size());
            Renderer::renderSystem[0].AddMesh(triangles, vertices, normals, uvs);
        }
    }
    std::vector<Entity> glasses(1024 * 128);
    Renderer::renderSystem[0].ReserveRenderObjects(glasses.size() + 6 + 128);
    int r = ceil(pow(glasses.size(), 1.0 / 3.0));
    for (int i = 0; i < glasses.size(); i++)
    {
        glasses[i] = Entity::AddEntity(Transform({ i % r * 1.37 * 12.0 + 250,(i / r) % r * 1.37 * 12.0 + 250,i / r / r * 1.37 * 12.0 }, { 12.0,12.0,12.0 }), MeshArray(0, 3, 2));
    }

    Entity e[6];
    for (int i = 0; i < 6; i++)
        e[i] = Entity::AddEntity(Transform({ 100,0,0 }), MeshArray(0, 3 + i, 2 + i));

    std::vector<Entity> rigidBodies;
    rigidBodies.reserve(128);
    rigidBodies.push_back(Entity::AddEntity(
        Transform({ 0,0,-0.25 }, { 10,10,0.5 }),
        Collider({ 10,10,0.5 }),
        RigidBody({ 0,0,0 }, { 0,0,0 }, { 0,0,0 }, std::numeric_limits<double>::infinity(), glm::mat3(std::numeric_limits<double>::infinity()), 1.0),
        MeshArray(0, 0, 1)
    ));
    rigidBodies.push_back(Entity::AddEntity(
        Transform({ 10.501,0,10.26 }, { 0.5,10,10 }),
        Collider({ 0.5,10,10 }),
        RigidBody({ 0,0,0 }, { 0,0,0 }, { 0,0,0 }, std::numeric_limits<double>::infinity(), glm::mat3(std::numeric_limits<double>::infinity()), 0.2, 0.1),
        MeshArray(0, 0, 1))

    );
    rigidBodies.push_back(Entity::AddEntity(
        Transform({ -10.501,0,10.26 }, { 0.5,10,10 }),
        Collider({ 0.5,10,10 }),
        RigidBody({ 0,0,0 }, { 0,0,0 }, { 0,0,0 }, std::numeric_limits<double>::infinity(), glm::dmat3(std::numeric_limits<double>::infinity()), 0.2, 0.1),
        MeshArray(0, 0, 1))

    );
    rigidBodies.push_back(Entity::AddEntity(
        Transform({ 0,10.501,10.26 }, { 10,0.5,10 }),
        Collider({ 10,0.5,10 }),
        RigidBody({ 0,0,0 }, { 0,0,0 }, { 0,0,0 }, std::numeric_limits<double>::infinity(), glm::dmat3(std::numeric_limits<double>::infinity()), 0.2, 0.1),
        MeshArray(0, 0, 1))

    );
    rigidBodies.push_back(Entity::AddEntity(
        Transform({ 0,-10.501,10.26 }, { 10,0.5,10 }),
        Collider({ 10,0.5,10 }),
        RigidBody({ 0,0,0 }, { 0,0,0 }, { 0,0,0 }, std::numeric_limits<double>::infinity(), glm::dmat3(std::numeric_limits<double>::infinity()), 0.2, 0.1),
        MeshArray(0, 0, 1)
    ));

    UISystem::Init(window, 2, GPUDrivenRendererSystem::renderPass);

    Transform cameraTransform({ 94.5,-25.0,14.5 }, { 1.0,1.0,1.0 }, glm::angleAxis(glm::radians(-15.0), glm::dvec3{ 1.0,0.0,0.0 }));
    static float fov = 70;
    glfwSetScrollCallback(window, [](GLFWwindow* window, double scrollx, double scrolly)
        {
            fov = fov - pow(scrolly, 3);
            if (fov < 0) fov = 0;
            if (fov > 180) fov = 180;
        });
    glm::dvec2 lastMousePos;
    glfwGetCursorPos(window, &lastMousePos.x, &lastMousePos.y);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, true);

        {
            glm::dvec2 mousePos;
            glfwGetCursorPos(window, &mousePos.x, &mousePos.y);
            auto mouseDelta = (lastMousePos - mousePos) * 0.00006 * (double) fov;
            lastMousePos = mousePos;

            if (!ImGui::GetIO().WantCaptureMouse)
            {
                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
                {
                    auto rotation = glm::rotate(cameraTransform.rotation, mouseDelta.x, { 0,0,1 });
                    cameraTransform.rotation = glm::rotate(rotation, mouseDelta.y, { 1,0,0 });
                }

                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
                {
                    auto rotation = glm::rotate(rigidBodies[0].GetComponent<Transform>().rotation, mouseDelta.x, { 0,1,0 });
                    rigidBodies[0].GetComponent<Transform>().rotation = glm::rotate(rotation, mouseDelta.y, { 1,0,0 });
                }
            }

            double speed = 0.06;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
                speed *= 0.2;
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
                speed *= 6;

            if (glfwGetKey(window, GLFW_KEY_Q))
                cameraTransform.position -= cameraTransform.Up() * speed;

            if (glfwGetKey(window, GLFW_KEY_E))
                cameraTransform.position += cameraTransform.Up() * speed;

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

        if (!SettingsSystem::stop || SettingsSystem::doAStep)
        {
            SettingsSystem::doAStep = false;
            Physics::restitutionMult = SettingsSystem::restitutionMultiplier;
            Physics::staticFrictionMult = SettingsSystem::staticFrictionMultiplier;
            Physics::dynamicFrictionMult = SettingsSystem::dynamicFrictionMultiplier;

            Physics::Update();

            if (SettingsSystem::shouldAddObject)
            {
                SettingsSystem::shouldAddObject = false;
                int type = rand() % 2;
                dvec3 pos;
                pos.x = rand() / double(RAND_MAX) * 7 - 3.5;
                pos.y = rand() / double(RAND_MAX) * 7 - 3.5;
                pos.z = 35;
                dvec3 scale;
                scale.x = rand() / double(RAND_MAX) * 1.5 + 1.75;
                scale.y = rand() / double(RAND_MAX) * 1.5 + 1.75;
                scale.z = rand() / double(RAND_MAX) * 1.5 + 1.75;
                dvec3 velocity;
                velocity.x = rand() / double(RAND_MAX) * 2 - 1;
                velocity.y = rand() / double(RAND_MAX) * 2 - 1;
                velocity.z = rand() / double(RAND_MAX) * 2 - 1;
                if (type == 0)
                {
                    double mass = 4.0 / 3.0 * glm::pi<double>() * scale.x * scale.x * 0.2;
                    rigidBodies.push_back(Entity::AddEntity(
                        Transform(pos, { scale.x,scale.x,scale.x }),
                        Collider({ scale.x }),
                        RigidBody(velocity, { 0,0,0 }, { 0,0,0 }, mass, glm::dmat3(2.0 / 5.0), 0.25),
                        MeshArray(0, rand() % 3, type)
                    ));
                }
                if (type == 1)
                {
                    double mass = scale.x * 2.0 * scale.y * 2.0 * scale.z * 2.0;
                    rigidBodies.push_back(Entity::AddEntity(
                        Transform(pos, scale),
                        Collider(scale),
                        RigidBody(velocity, { 0,0,0 }, { 0,0,0 }, mass, glm::dmat3(1.0 / 12.0 * (pow(scale.x * 2.0, 2.0) + pow(scale.z * 2.0, 2.0)), 0, 0, 0, 1.0 / 12.0 * (pow(scale.y * 2.0, 2.0) + pow(scale.z * 2.0, 2.0)), 0, 0, 0, 1.0 / 12.0 * (pow(scale.x * 2.0, 2.0) + pow(scale.y * 2.0, 2.0))), 0.25),
                        MeshArray(0, rand() % 3, type)
                    ));
                }
            }
        }
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        Renderer::DrawFrame(cameraTransform, glm::radians(fov));
        if (glfwGetKey(window, GLFW_KEY_I)) drawUI = true;
        if (glfwGetKey(window, GLFW_KEY_U)) drawUI = false;

        if (drawUI)
        {
            UISystem::Begin();
            SettingsSystem::Run();
            IDDisplaySystem::Run(cameraTransform, glm::radians(fov), (float) w / (float) h);
            CollisionDisplaySystem::Run();
            LoggerSystem::Run();
            UISystem::End(Renderer::GetCurrentCmdBuffer());
        }
        Renderer::Present(generalQueue);
    }
    UISystem::Destroy();
    Renderer::Destroy();
    glfwTerminate();
}