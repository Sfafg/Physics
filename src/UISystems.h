#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "vulkan/vulkan.hpp"
#include "Components.h"
#include "ColliderManager.h"
#include "imgui/imconfig.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#include <iostream>
#include <vector>

class UISystem
{
    static vg::DescriptorPool descriptorPool;
public:
    static ImFont* font;
    static void Init(GLFWwindow* window, int imageCount, vg::RenderPass& renderPass)
    {
        descriptorPool = vg::DescriptorPool(1000, {
            { vg::DescriptorType::Sampler, 1000 },
            { vg::DescriptorType::CombinedImageSampler, 1000 },
            { vg::DescriptorType::SampledImage, 1000 },
            { vg::DescriptorType::StorageImage, 1000 },
            { vg::DescriptorType::UniformTexelBuffer, 1000 },
            { vg::DescriptorType::StorageTexelBuffer, 1000 },
            { vg::DescriptorType::UniformBuffer, 1000 },
            { vg::DescriptorType::StorageBuffer, 1000 },
            { vg::DescriptorType::UniformBufferDynamic, 1000 },
            { vg::DescriptorType::StorageBufferDynamic, 1000 },
            { vg::DescriptorType::InputAttachment, 1000 }
            });

        // Init ImGUI
        vg::PhysicalDeviceHandle physicalDevice = *vg::currentDevice;
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::GetIO().LogFilename = nullptr;
        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo info{};
        info.Instance = *(VkInstance*) &(vg::InstanceHandle&) vg::instance;
        info.PhysicalDevice = *(VkPhysicalDevice*) &physicalDevice;
        info.Device = *(VkDevice*) &(vg::DeviceHandle&) *vg::currentDevice;
        info.Queue = *(VkQueue*) &(vg::QueueHandle&) vg::currentDevice->GetQueue(0);
        info.DescriptorPool = *(VkDescriptorPool*) &(vg::DescriptorPoolHandle&) descriptorPool;
        info.RenderPass = *(VkRenderPass*) &(vg::RenderPassHandle&) renderPass;
        info.MinImageCount = 2;
        info.ImageCount = imageCount;
        info.MSAASamples = (VkSampleCountFlagBits) 1;

        ImGuiIO& io = ImGui::GetIO();
        font = io.Fonts->AddFontFromFileTTF("resources/LibreBaskerville-Regular.ttf", 24.0f);

        ImGui_ImplVulkan_Init(&info);
        ImGui_ImplVulkan_CreateFontsTexture();
    }

    static void Begin()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    static void End(vg::CmdBuffer& cmdBuffer)
    {
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *(VkCommandBuffer*) &(vg::CmdBufferHandle&) cmdBuffer, 0);
    }

    static void Destroy()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
};
vg::DescriptorPool UISystem::descriptorPool;
ImFont* UISystem::font;

class SettingsSystem
{
public:
    static int futureStepCount;
    static int subStepCount;
    static float deltaT;
    static float restitutionMultiplier;
    static float staticFrictionMultiplier;
    static float dynamicFrictionMultiplier;
    static bool stop;
    static bool doAStep;
    static bool settingsChanged;
    static bool shouldAddObject;
    static void Run()
    {
        ImGui::Begin("Ustawienia");
        ImGui::SetWindowPos(ImVec2(1920 - 300, 50), ImGuiCond_FirstUseEver);
        ImGui::SetWindowFontScale(0.8f);

        int futureStepCount_ = futureStepCount;
        int subStepCount_ = subStepCount;
        float deltaT_ = deltaT;
        ImGui::SliderInt("Liczba przewidywanych krokow", &futureStepCount, 0, 200);
        ImGui::SliderInt("Liczba pod krokow", &subStepCount, 1, 16);
        ImGui::SliderFloat("DeltaT", &deltaT, 0.00001f, 0.1f);
        ImGui::SliderFloat("Restytucja", &restitutionMultiplier, 0.0f, 1.0f);
        ImGui::SliderFloat("Tarcie statyczne", &staticFrictionMultiplier, 0.0f, 1.0f);
        ImGui::SliderFloat("Tarcie dynamiczne", &dynamicFrictionMultiplier, 0.0f, 1.0f);
        ImGui::Checkbox("Stop", &stop);

        static bool bReleased = false;
        if (ImGui::Button("Przejdz o krok"))
        {
            stop = true;
            if (bReleased)
            {
                doAStep = true;
                bReleased = false;
            }
        }
        else bReleased = true;

        static bool objectButtonReleased = false;
        if (ImGui::Button("Dodaj obiekt"))
        {
            if (objectButtonReleased)
            {
                shouldAddObject = true;
                objectButtonReleased = false;
            }
        }
        else objectButtonReleased = true;

        settingsChanged =
            futureStepCount_ != futureStepCount ||
            subStepCount_ != subStepCount ||
            deltaT_ != deltaT;

        Physics::deltaT = deltaT;
        Physics::subStepCount = subStepCount;

        ImGui::End();
    }
};
int SettingsSystem::futureStepCount = 0;
int SettingsSystem::subStepCount = 8;
float SettingsSystem::deltaT = 1.0 / 70.0;
float SettingsSystem::restitutionMultiplier = 1;
float SettingsSystem::staticFrictionMultiplier = 1;
float SettingsSystem::dynamicFrictionMultiplier = 1;
bool SettingsSystem::stop = false;
bool SettingsSystem::doAStep = false;
bool SettingsSystem::settingsChanged = true;
bool SettingsSystem::shouldAddObject = true;

class IDDisplaySystem : public ECS::System<Collider>
{
    static glm::vec2 TransformToClipSpace(const glm::mat4& cameraMatrix, glm::vec3 point)
    {
        glm::vec4 p = cameraMatrix * glm::vec4(point, 1.0);
        bool isBehind = p.w < 0;
        p /= p.w;

        glm::vec2 p1 = p;
        if (isBehind)
            p1 = -p1;
        if (std::abs(p1.x) > 1 || std::abs(p1.y) > 1 || isBehind)
        {
            if (std::abs(p1.x) > std::abs(p1.y))
                p1.x = p1.x > 0 ? 1 : -1;
            else
                p1.y = p1.y > 0 ? 1 : -1;
        }

        return glm::clamp((p1 + glm::vec2(1, 1)) * 0.5f, glm::vec2(0, 0), glm::vec2(1, 1));
    }

public:
    static void Run(Transform cameraTransform, float fov, float ratio)
    {
        glm::mat4 cameraView = glm::lookAt(cameraTransform.position, cameraTransform.position + cameraTransform.Forward(), cameraTransform.Up());
        glm::mat4 cameraProjection = glm::perspective(fov, ratio, 0.01f, 1000.0f);
        cameraProjection[1][1] *= -1;
        glm::mat4 cameraMatrix = cameraProjection * cameraView;

        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("OverlayWindow", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize);

        glm::vec2 windowSize(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        std::vector<glm::vec2> labelPositions;
        for (auto&& i : components)
        {
            glm::vec2 pos = TransformToClipSpace(cameraMatrix, i.GetComponent<Transform>().position) * windowSize;
            labelPositions.push_back(pos);
        }

        ImGui::PushFont(UISystem::font);
        for (auto&& pos : labelPositions)
        {
            glm::vec2 direction(0, 0);

            for (auto&& pos1 : labelPositions)
            {
                glm::vec2 d = pos1 - (pos + direction);
                float sqrDist = glm::dot(d, d);
                if (sqrDist != 0)
                    direction -= (d / sqrt(sqrDist)) * 10.0f;
            }

            float dirLength = glm::length(direction);
            direction *= 60.0f / dirLength;

            glm::vec2 start = pos;
            glm::vec2 mid = glm::clamp(start + direction, glm::vec2(50, 50), windowSize - glm::vec2(50, 50));
            direction = mid - start;
            glm::vec2 end = mid + glm::vec2(40, 0) * (direction.x < 0 ? -1.0f : 1.0f);

            std::string label = std::to_string(&pos - &labelPositions[0]);
            drawList->AddLine(*(ImVec2*) &start, *(ImVec2*) &mid, IM_COL32(255, 255, 255, 255), 1.0f);
            drawList->AddLine(*(ImVec2*) &mid, *(ImVec2*) &end, IM_COL32(255, 255, 255, 255), 1.0f);

            glm::vec2 textPos = (mid + end) * 0.5f + glm::vec2(0, -24);
            drawList->AddText(*(ImVec2*) &textPos, IM_COL32(255, 255, 255, 255), &label[0]);
        }
        ImGui::PopFont();

        ImGui::End();
    }
};

class LoggerSystem
{
public:
    static std::string text;

    static void Run()
    {
        ImGui::Begin("Log Kolizji", 0, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowPos(ImVec2(50, 500), ImGuiCond_FirstUseEver);

        ImGui::SetWindowFontScale(0.7f);
        if (ImGui::Button("Clear", ImVec2(220, 20)))
            text.clear();

        size_t pos = text.size() - 1;
        size_t newPos;
        while ((newPos = text.rfind('\n', pos - 1)) != std::string::npos)
        {
            ImGui::Text(text.substr(newPos + 1, pos - newPos - 1).c_str());
            pos = newPos;
        }
        ImGui::Text(text.substr(0, pos).c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::End();
    }
};
std::string LoggerSystem::text;

class CollisionDisplaySystem : public ECS::System<Collider>
{
public:
    static void Run()
    {
        int futureStepCount = SettingsSystem::futureStepCount;
        const int cellSize = 28;
        static std::unordered_set<CollisionPair, CollisionPair::Hash> previouslyInCollision;
        std::vector<std::unordered_set<CollisionPair, CollisionPair::Hash>> inCollisionAtStep;

        ImGui::Begin("Macierz Kolizji", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
        ImGui::SetWindowSize(ImVec2(cellSize * (components.size() + 1), cellSize * (components.size() + 1) + 46), ImGuiCond_Always);

        ImGui::PushFont(UISystem::font);
        if (ImGui::BeginTable("CollisionsTable", components.size() + 1, ImGuiTableFlags_Borders))
        {
            auto rigidBodyState = ECS::System<RigidBody>::components;
            auto transformState = ECS::System<Transform>::components;

            std::unordered_set<CollisionPair, CollisionPair::Hash> futurePossibleColliders;
            for (auto&& i : components)
                ColliderManager::QuarryInRadius(i, 1 + glm::length(i.GetComponent<RigidBody>().velocity) * Physics::deltaT * 2 * futureStepCount, &futurePossibleColliders);

            for (int n = 0; n <= futureStepCount; n++)
            {
                std::unordered_set<CollisionPair, CollisionPair::Hash> possibleColliders;
                for (auto&& i : components)
                    ColliderManager::QuarryInRadius(i, 1 + glm::length(i.GetComponent<RigidBody>().velocity) * Physics::deltaT * 2, &possibleColliders);

                inCollisionAtStep.push_back(std::unordered_set<CollisionPair, CollisionPair::Hash>());
                for (int k = 0; k < Physics::subStepCount; k++)
                {
                    if (futurePossibleColliders.size() == 0);

                    std::unordered_set<CollisionPair, CollisionPair::Hash> collisions;
                    for (auto&& i : possibleColliders)
                        if (ColliderManager::AreColliding(*i.a, *i.b))
                            collisions.insert(i);

                    Physics::Substep(Physics::deltaT / Physics::subStepCount, collisions);

                    for (auto&& i : collisions)
                    {
                        if (futurePossibleColliders.contains(i))
                        {
                            inCollisionAtStep[n].insert(i);
                            futurePossibleColliders.erase(i);
                        }
                    }
                }
            }

            ECS::System<RigidBody>::components = rigidBodyState;
            ECS::System<Transform>::components = transformState;

            ImGui::TableNextColumn();
            for (int k = 0; k < components.size(); k++)
            {
                ImGui::TableNextColumn();
                ImGui::Text(std::to_string(k).c_str());
            }
            ImGui::TableNextRow();
            for (int i = 0; i < components.size(); i++)
            {
                ImGui::TableNextColumn();
                ImGui::Text(std::to_string(i).c_str());
                for (int j = 0; j < components.size(); j++)
                {
                    ImGui::TableNextColumn();
                    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2((cellSize) / 2.0f, (cellSize) / 2.0f));

                    ImColor color(0, 255, 0);
                    CollisionPair pair(&components[i], &components[j]);
                    bool previouslyColliding = previouslyInCollision.contains(pair) || previouslyInCollision.contains(pair.Inverse());
                    for (int n = 0; n <= futureStepCount; n++)
                    {
                        bool colliding = inCollisionAtStep[n].contains(pair) || inCollisionAtStep[n].contains(pair.Inverse());
                        if (i < j)
                        {
                            if (colliding)
                            {
                                if (n == 0)
                                {
                                    if (!previouslyColliding)
                                        LoggerSystem::text += "Para (" + std::to_string(i) + "," + std::to_string(j) + ") wchodzi w kolizje\n";
                                }
                                else
                                    LoggerSystem::text += "Para (" + std::to_string(i) + "," + std::to_string(j) + ") wejdzie w kolizje za " + std::to_string(n) + "\n";
                            }
                            else if (n == 0 && previouslyColliding)
                                LoggerSystem::text += "Para (" + std::to_string(i) + "," + std::to_string(j) + ") wychodzi z kolizji\n";
                        }

                        if (colliding)
                        {
                            float r, g, b;
                            ImGui::ColorConvertHSVtoRGB(n / (float) (futureStepCount + 1) * 0.3333f, 1, 1, r, g, b);
                            color = ImColor(r, g, b);
                            break;
                        }
                    }

                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, color);

                    ImGui::PopStyleVar();
                }
            }
            previouslyInCollision = std::move(inCollisionAtStep[0]);
            ImGui::EndTable();
        }
        ImGui::PopFont();
        ImGui::End();
    }
};