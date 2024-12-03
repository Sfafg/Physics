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

        std::unordered_set<CollisionPair, CollisionPair::Hash> possibleColliders;
        for (auto&& i : components)
            ColliderManager::QuerryInRadius(i.GetComponent<Collider>(), 1 + glm::length(i.velocity) * deltaT * 2, &possibleColliders);

        for (int i = 0; i < subStepCount; i++)
        {
            // Integration
            for (int j = 0; j < components.size(); j++)
            {
                components[j].AddForce(gravity, subDeltaT);
                components[j].Integrate(subDeltaT);
            }

            std::vector<PenetrationConstraint> penetrations;//= ColliderManager::GetPenetrations();
            for (auto&& j : possibleColliders)
            {
                PenetrationConstraint t;
                if (ColliderManager::GetPenetration(*j.a, *j.b, &t))
                    penetrations.emplace_back(std::move(t));
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
