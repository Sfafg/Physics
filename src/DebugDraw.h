#pragma once
#include "Renderer.h"

using namespace ECS;
class DebugDraw
{
    static glm::vec3 triangleNormal(glm::vec3 a, glm::vec3 b, glm::vec3 c)
    {
        return glm::normalize(glm::cross(a - b, a - c));
    }

    static Mesh CreateLineMesh(const vg::Queue& queue)
    {
        const glm::vec3 a = { 0,0,0 };
        const glm::vec3 b = { 0,0,1 };
        const float thickness = 0.5f;

        glm::vec3 crossA = { 1,0,0 };
        glm::vec3 crossB = { 0,1,0 };
        crossA *= thickness;
        crossB *= thickness;

        std::vector<glm::vec3> pos(8);

        pos[0] = a - crossA - crossB;
        pos[1] = a - crossA + crossB;
        pos[2] = a + crossA - crossB;
        pos[3] = a + crossA + crossB;
        pos[4] = b - crossA - crossB;
        pos[5] = b - crossA + crossB;
        pos[6] = b + crossA - crossB;
        pos[7] = b + crossA + crossB;

        std::vector<uint16_t> index = { 0,1,2,3,7,1,5, 0, 4, 2, 6, 7, 4, 5 };

        bool reversedWinding = false;
        std::vector<glm::vec3> norm(pos.size());
        for (int i = 0; i < index.size() - 3; i++)
        {
            if (index[i + 2] == (uint16_t) -1)
            {
                i += 2;
                reversedWinding = false;
                continue;
            }
            auto normal = (reversedWinding ? -1.0f : 1.0f) * triangleNormal(pos[index[i]], pos[index[i + 1]], pos[index[i + 2]]);
            norm[index[i]] += normal;
            norm[index[i + 1]] += normal;
            norm[index[i + 2]] += normal;
            reversedWinding = !reversedWinding;
        }
        for (int i = 0; i < norm.size(); i++)
            norm[i] = glm::normalize(norm[i]);

        BufferArray positions(pos, { vg::BufferUsage::VertexBuffer, vg::BufferUsage::TransferDst }, vg::MemoryProperty::DeviceLocal, &queue);
        BufferArray normals(norm, { vg::BufferUsage::VertexBuffer, vg::BufferUsage::TransferDst }, vg::MemoryProperty::DeviceLocal, &queue);
        BufferArray indices(index, { vg::BufferUsage::IndexBuffer, vg::BufferUsage::TransferDst }, vg::MemoryProperty::DeviceLocal, &queue);

        return Mesh({ positions,normals }, indices, vg::IndexType::Uint16, index.size(), 1);
    }

    static Mesh CreatePiramidMesh(const vg::Queue& queue)
    {
        std::vector<glm::vec3> pos(5);

        pos[0] = { -0.5f,-0.5f,0.0f };
        pos[1] = { -0.5f, 0.5f,0.0f };
        pos[2] = { 0.5f,-0.5f,0.0f };
        pos[3] = { 0.5f, 0.5f,0.0f };
        pos[4] = { 0.0f, 0.0f,1.0f };

        std::vector<uint16_t> index = { 0,1,2,3,4,1,0,uint16_t(-1),0,2,4 };

        bool reversedWinding = false;
        std::vector<glm::vec3> norm(pos.size());
        for (int i = 0; i < index.size() - 3; i++)
        {
            if (index[i + 2] == (uint16_t) -1)
            {
                i += 2;
                reversedWinding = false;
                continue;
            }
            auto normal = (reversedWinding ? -1.0f : 1.0f) * triangleNormal(pos[index[i]], pos[index[i + 1]], pos[index[i + 2]]);
            norm[index[i]] += normal;
            norm[index[i + 1]] += normal;
            norm[index[i + 2]] += normal;
            reversedWinding = !reversedWinding;
        }
        for (int i = 0; i < norm.size(); i++)
            norm[i] = glm::normalize(norm[i]);

        BufferArray positions(pos, { vg::BufferUsage::VertexBuffer, vg::BufferUsage::TransferDst }, vg::MemoryProperty::DeviceLocal, &queue);
        BufferArray normals(norm, { vg::BufferUsage::VertexBuffer, vg::BufferUsage::TransferDst }, vg::MemoryProperty::DeviceLocal, &queue);
        BufferArray indices(index, { vg::BufferUsage::IndexBuffer, vg::BufferUsage::TransferDst }, vg::MemoryProperty::DeviceLocal, &queue);

        return Mesh({ positions,normals }, indices, vg::IndexType::Uint16, index.size(), 1);
    }

    static Mesh CreateSphereMesh(const vg::Queue& queue)
    {
        std::vector<glm::vec3> pos;
        std::vector<glm::vec3> norm;
        std::vector<uint16_t> index;

        const int resolution = 12;
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
                pos.push_back(p);
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

        BufferArray positions(pos, { vg::BufferUsage::VertexBuffer, vg::BufferUsage::TransferDst }, vg::MemoryProperty::DeviceLocal, &queue);
        BufferArray normals(norm, { vg::BufferUsage::VertexBuffer, vg::BufferUsage::TransferDst }, vg::MemoryProperty::DeviceLocal, &queue);
        BufferArray indices(index, { vg::BufferUsage::IndexBuffer, vg::BufferUsage::TransferDst }, vg::MemoryProperty::DeviceLocal, &queue);

        return Mesh({ positions, normals }, indices, vg::IndexType::Uint16, index.size(), 1);
    };

public:
    static Mesh lineMesh;
    static Mesh piramidMesh;
    static Mesh sphereMesh;
public:
    static glm::vec4 color;
    static glm::mat4 matrix;

    static void Init(const vg::Queue& queue)
    {
        lineMesh = CreateLineMesh(queue);
        piramidMesh = CreatePiramidMesh(queue);
        sphereMesh = CreateSphereMesh(queue);
    }

    static void Free()
    {
        lineMesh.~Mesh();
        piramidMesh.~Mesh();
        sphereMesh.~Mesh();
    }


    static void Line(glm::vec3 a, glm::vec3 b, float thickness = 0.1f, int frameCount = 1)
    {
        float distance = glm::distance(a, b);
        glm::vec3 direction = (b - a) / distance;
        glm::vec3 rotationAxis = glm::cross({ 0,0,1 }, direction);
        if (rotationAxis == glm::vec3{ 0,0,0 })
            rotationAxis = { 1,0,0 };

        float rotationAngle = acos(glm::dot(direction, { 0,0,1 }));
        glm::mat4 mat = glm::translate(glm::mat4(1), a) * glm::mat4(glm::angleAxis(rotationAngle, glm::normalize(rotationAxis))) * glm::scale(glm::mat4(1), { thickness,thickness,distance });

        Renderer::DrawMesh(lineMesh, std::tuple{ color, matrix * mat }, frameCount);
    }

    static void Triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c, int frameCount = 1)
    {
        glm::vec3 normal = triangleNormal(a, b, c);
        BufferArray positions(std::vector<glm::vec3>{ a, b, c }, vg::BufferUsage::VertexBuffer, vg::MemoryProperty::HostVisible);
        BufferArray normals(std::vector<glm::vec3>{normal, normal, normal}, vg::BufferUsage::VertexBuffer, vg::MemoryProperty::HostVisible);
        BufferArray indices(std::vector<uint16_t>{ 0, 1, 2 }, vg::BufferUsage::IndexBuffer, vg::MemoryProperty::HostVisible);

        Renderer::DrawMesh(Mesh({ positions,normals }, indices, vg::IndexType::Uint16, 3, 1), std::tuple{ color, matrix }, frameCount);
    }

    static void Arrow(glm::vec3 a, glm::vec3 b, float thickness = 0.1f, int frameCount = 1)
    {
        float distance = glm::distance(a, b);
        glm::vec3 direction = (b - a) / distance;

        Line(a, b - direction * 4.0f * thickness, thickness, frameCount);


        glm::vec3 rotationAxis = glm::cross({ 0,0,1 }, direction);
        if (rotationAxis == glm::vec3{ 0,0,0 })
            rotationAxis = { 1,0,0 };

        float rotationAngle = acos(glm::dot(direction, { 0,0,1 }));
        glm::mat4 mat = glm::translate(glm::mat4(1), b - direction * 4.0f * thickness) * glm::mat4(glm::angleAxis(rotationAngle, glm::normalize(rotationAxis))) * glm::scale(glm::mat4(1), { thickness * 2.0f,thickness * 2.0f,thickness * 4.0f });

        Renderer::DrawMesh(piramidMesh, std::tuple{ color, matrix * mat }, frameCount);
    }

    static void Ray(glm::vec3 origin, glm::vec3 direction, float thickness = 0.1f, int frameCount = 1)
    {
        Arrow(origin, origin + direction, thickness, frameCount);
    }

    static void Sphere(glm::vec3 position, float radius, int frameCount = 1)
    {
        glm::mat4 mat = glm::translate(glm::mat4(1), position) * glm::scale(glm::mat4(1), { radius,radius,radius });
        Renderer::DrawMesh(sphereMesh, std::tuple{ color, matrix * mat }, frameCount);
    }

    static void WireSphere(glm::vec3 position, float radius, int resolution = 16, float thickness = 0.002f, int frameCount = 1)
    {
        std::vector<glm::vec3> pos;
        std::vector<glm::vec3> norm;
        std::vector<glm::vec3> col;
        std::vector<uint16_t> index;

        const float PI = 3.14159265359f;
        for (int i = 0; i < resolution; i++)
        {
            float a = (i / (float) resolution) * PI * 2;

            glm::vec3 c{ radius * sin(a), radius * cos(a), 0 };
            auto crossA = glm::normalize(c) * thickness;
            auto crossB = glm::vec3{ 0,0,1 } *thickness;

            pos.push_back(position + c - crossA - crossB);
            pos.push_back(position + c + crossA - crossB);
            pos.push_back(position + c - crossA + crossB);
            pos.push_back(position + c + crossA + crossB);

            norm.push_back(crossA);
            norm.push_back(crossA);
            norm.push_back(-crossA);
            norm.push_back(-crossA);

            col.push_back(color);
            col.push_back(color);
            col.push_back(color);
            col.push_back(color);

            auto vec = std::vector<int>{ 4, 0, 5, 1, 7, 3, 6, 2, 4, 0, uint16_t(-1) };
            for (auto& ve : vec)
                if (ve != uint16_t(-1))
                    ve = (i * 4 + ve) % (resolution * 4);

            index.insert(index.end(), vec.begin(), vec.end());
        }

        for (int i = 0; i < resolution; i++)
        {
            float a = (i / (float) resolution) * PI * 2;

            glm::vec3 c{ radius * sin(a), 0, radius * cos(a) };
            auto crossA = glm::normalize(c) * thickness;
            auto crossB = glm::vec3{ 0,1,0 } *thickness;

            pos.push_back(position + c - crossA - crossB);
            pos.push_back(position + c + crossA - crossB);
            pos.push_back(position + c - crossA + crossB);
            pos.push_back(position + c + crossA + crossB);

            norm.push_back(crossA);
            norm.push_back(crossA);
            norm.push_back(-crossA);
            norm.push_back(-crossA);

            col.push_back(color);
            col.push_back(color);
            col.push_back(color);
            col.push_back(color);

            auto vec = std::vector<int>{ 4, 0, 5, 1, 7, 3, 6, 2, 4, 0, uint16_t(-1) };
            for (auto& ve : vec)
                if (ve != uint16_t(-1))
                    ve = resolution * 4 + (i * 4 + ve) % (resolution * 4);

            index.insert(index.end(), vec.begin(), vec.end());
        }

        for (int i = 0; i < resolution; i++)
        {
            float a = (i / (float) resolution) * PI * 2;

            glm::vec3 c{ 0, radius * sin(a), radius * cos(a) };
            auto crossA = glm::normalize(c) * thickness;
            auto crossB = glm::vec3{ 1,0,0 } *thickness;

            pos.push_back(position + c - crossA - crossB);
            pos.push_back(position + c + crossA - crossB);
            pos.push_back(position + c - crossA + crossB);
            pos.push_back(position + c + crossA + crossB);

            norm.push_back(crossA);
            norm.push_back(crossA);
            norm.push_back(-crossA);
            norm.push_back(-crossA);

            col.push_back(color);
            col.push_back(color);
            col.push_back(color);
            col.push_back(color);

            auto vec = std::vector<int>{ 4, 0, 5, 1, 7, 3, 6, 2, 4, 0, uint16_t(-1) };
            for (auto& ve : vec)
                if (ve != uint16_t(-1))
                    ve = resolution * 8 + (i * 4 + ve) % (resolution * 4);

            index.insert(index.end(), vec.begin(), vec.end());
        }

        BufferArray positions(pos, vg::BufferUsage::VertexBuffer, vg::MemoryProperty::HostVisible);
        BufferArray normals(norm, vg::BufferUsage::VertexBuffer, vg::MemoryProperty::HostVisible);
        BufferArray colors(col, vg::BufferUsage::VertexBuffer, vg::MemoryProperty::HostVisible);
        BufferArray indices(index, vg::BufferUsage::IndexBuffer, vg::MemoryProperty::HostVisible);

        Renderer::DrawMesh(Mesh({ positions, normals, colors }, indices, vg::IndexType::Uint16, index.size(), 1), std::tuple{ color,matrix }, frameCount);
    }
};
inline Mesh DebugDraw::lineMesh;
inline Mesh DebugDraw::piramidMesh;
inline Mesh DebugDraw::sphereMesh;
inline glm::vec4 DebugDraw::color = { 1,1,1,1 };
inline glm::mat4 DebugDraw::matrix = glm::mat4(1);
