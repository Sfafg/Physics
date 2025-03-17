#include "ObjLoader.h"
#include <iostream>
#include <fstream>
#include <string>
#include <istream>

const std::string vertexPrefix = "v";
const std::string normalPrefix = "vn";
const std::string facePrefix = "f";

std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<unsigned int>> LoadMesh(const char* meshPath)
{
    std::vector<glm::vec3> a;
    std::vector<glm::vec3> b;
    std::vector<unsigned int> c;
    if (!LoadMesh(meshPath, &a, &b, &c))
        std::cout << "Unable to load " << meshPath << '\n';
    return std::make_tuple(a, b, c);
}
bool LoadMesh(const char* meshPath, std::vector<glm::vec3>* vertices, std::vector<glm::vec3>* normals, std::vector<unsigned int>* triangles)
{
    std::ifstream meshFile(meshPath);
    if (!meshFile.is_open())
        return false;

    // Load mesh data.
    std::vector<glm::vec3> uniqueNormals;
    std::string prefix;
    meshFile >> prefix;
    while (!meshFile.eof() && prefix != facePrefix)
    {
        if (prefix == vertexPrefix || prefix == normalPrefix)
        {
            glm::vec3 vector;
            meshFile >> vector.x >> vector.y >> vector.z;

            if (prefix == vertexPrefix)
                vertices->push_back(vector);
            else
                uniqueNormals.push_back(vector);
        }
        meshFile >> prefix;
    }
    normals->resize(vertices->size());

    // Load face data.
    while (!meshFile.eof())
    {
        if (prefix == facePrefix)
        {
            unsigned int indices[3]{};
            for (int i = 0; i < 3; i++)
            {
                unsigned int vertexIndex, uvIndex, normalIndex;
                meshFile >> vertexIndex;
                meshFile.get();
                meshFile >> uvIndex;
                meshFile.clear();
                meshFile.get();
                meshFile >> normalIndex;
                meshFile.clear();

                vertexIndex--;
                uvIndex--;
                normalIndex--;

                indices[i] = vertexIndex;
                (*normals)[vertexIndex] = uniqueNormals[normalIndex];
            }
            triangles->push_back(indices[2]);
            triangles->push_back(indices[1]);
            triangles->push_back(indices[0]);
        }
        meshFile >> prefix;
    }
    return true;
}