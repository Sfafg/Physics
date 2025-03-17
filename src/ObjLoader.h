#pragma once
#include <vector>
#include <glm/glm.hpp>

/// @brief Funkcja wczytująca dane z pliku .obj, wspiera siatki złożone tylko z trójkątów, by uzyskać twarde normalne trzeba zduplikować punkty siatki.
/// @param meshPath 
/// @param vertices 
/// @param normals 
/// @param triangles 
/// @return 
bool LoadMesh(const char* meshPath, std::vector<glm::vec3>* vertices, std::vector<glm::vec3>* normals, std::vector<unsigned int>* triangles);
std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<unsigned int>> LoadMesh(const char* meshPath);