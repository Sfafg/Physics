#pragma once
#include "Components.h"
#include "ColliderManager.h"
#include <math.h>
#include <queue>

using dvec2 = glm::dvec2;
using dvec3 = glm::dvec3;
using dquat = glm::dquat;

class Physics : public ECS::System<RigidBody>
{
public:
    static dvec3 gravity;

    static void Update(double deltaT, int subStepCount = 4)
    {
        double subDeltaT = deltaT / subStepCount;

        std::vector<std::vector<Collider*>> possibleColliders;
        for (auto&& i : components)
            possibleColliders.push_back(ColliderManager::QuerryInRadius(i.GetComponent<Transform>().position, i.GetComponent<Collider>().boundingSphereRadius * glm::length(i.velocity) * deltaT * 2));

        for (int i = 0; i < subStepCount; i++)
        {
            // Integration
            for (int j = 0; j < components.size(); j++)
            {
                components[j].AddForce(gravity, subDeltaT);
                components[j].Integrate(subDeltaT);
            }

            std::vector<PenetrationConstraint> penetrations;// = ColliderManager::GetPenetrations();
            for (int j = 0; j < components.size(); j++)
            {
                auto&& t = ColliderManager::GetPenetrations(&components[j].GetComponent<Collider>(), possibleColliders[j]);
                for (auto&& i : t)
                    penetrations.push_back(std::move(i));
            }

            // Position Solve.
            for (auto&& penetration : penetrations)
                penetration.SolvePositions(subDeltaT);

            // Velocity update.
            for (int j = 0; j < components.size(); j++)
                components[j].UpdateVelocities(subDeltaT);

            // Velocity Solve.
            for (int j = 0; j < penetrations.size(); j++)
                penetrations[j].SolveVelocities(glm::length(gravity), subDeltaT);
        }
    }
};
dvec3 Physics::gravity = { 0,0,-9.81 };
