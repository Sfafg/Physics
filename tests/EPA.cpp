#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <math.h>
#include "VG/VG.h"
#include "Physics.h"

using namespace ECS;
using namespace vg;

int main()
{
    Entity rigidBodies[2];
    rigidBodies[0] = Entity::AddEntity(
        Transform({ 2,2,-0.25 }, { 1,1,0.25 }),
        Collider({ 1,1,0.25 }),
        RigidBody({ 0,0,0 }, { 0,0,0 }, { 0,0,0 }, std::numeric_limits<double>::infinity(), glm::mat3(1))
    );

    rigidBodies[1] = Entity::AddEntity(
        Transform({ 2,2,1 }, { 0.5,0.5,0.5 }),
        Collider({ 0.5 }),
        RigidBody({ 0,4,-2 }, { 0,0,0 }, { 0,0,0 }, 3.0, glm::mat3(2.0f / 3.0f), 1.0)
    );

    for (int i = 0; i < 10'000'000; i++)
    {
        rigidBodies[1].GetComponent<Transform>().position.x = rand() / (double) RAND_MAX * 1.5 + 1;
        rigidBodies[1].GetComponent<Transform>().position.y = rand() / (double) RAND_MAX * 1.5 + 1;
        rigidBodies[1].GetComponent<Transform>().position.z = (rand() / (double) RAND_MAX - 0.5) * 2 * 0.1 + 0.5;
        auto penetrations = ColliderManager::GetPenetrations();
        for (auto&& penetration : penetrations)
        {
            if (std::isnan(penetration.depth))
            {
                std::cout << "FAILED at position: "
                    << rigidBodies[1].GetComponent<Transform>().position.x << ", "
                    << rigidBodies[1].GetComponent<Transform>().position.y << ", "
                    << rigidBodies[1].GetComponent<Transform>().position.z << "\n";
                return i;
            }
        }
    }

    return 0;
}