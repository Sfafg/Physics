#pragma once
#include "Components.h"
#include "ColliderManager.h"
#include <math.h>
#include <queue>

using dvec2 = glm::dvec2;
using dvec3 = glm::dvec3;
using dquat = glm::dquat;

/// @brief System zajmujący się dynamiką obiektów sztywnych.
/// @details Działa na podstawie metody XPBD(Extended Position Based Dynamics), kolizje wykrywane są z pomocą klasy ColliderManager.
/// @ref ColliderManager
class Physics : public ECS::System<RigidBody>
{
public:
    static dvec3 gravity;
    static double deltaT;
    static int subStepCount;
    static double restitutionMult;
    static double dynamicFrictionMult;
    static double staticFrictionMult;

    static void Update()
    {
        double subDeltaT = deltaT / subStepCount;

        std::unordered_set<CollisionPair, CollisionPair::Hash> possibleColliders;
        for (auto&& i : components)
            ColliderManager::QuarryInRadius(i.GetComponent<Collider>(), 1 + glm::length(i.velocity) * deltaT * 2, &possibleColliders);

        for (int i = 0; i < subStepCount; i++)
        {
            Substep(subDeltaT, possibleColliders);
        }
    }

    static void Substep(double subDeltaT, std::unordered_set<CollisionPair, CollisionPair::Hash>& possibleColliders)
    {
        std::vector<double> originalRestitution(components.size());
        std::vector<double> originalStaticFriction(components.size());
        std::vector<double> originalDynamicFriction(components.size());

        for (int j = 0; j < components.size(); j++)
        {
            originalRestitution[j] = components[j].restitutionCoefficient;
            originalStaticFriction[j] = components[j].staticFrictionCoefficient;
            originalDynamicFriction[j] = components[j].dynamicFrictionCoefficient;

            components[j].restitutionCoefficient *= restitutionMult;
            components[j].staticFrictionCoefficient *= staticFrictionMult;
            components[j].dynamicFrictionCoefficient *= dynamicFrictionMult;
        }


        // Integration
        for (int j = 0; j < components.size(); j++)
        {
            components[j].AddForce(gravity, subDeltaT);
            components[j].Integrate(subDeltaT);
        }

        std::vector<PenetrationConstraint> penetrations;
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


        for (int j = 0; j < components.size(); j++)
        {
            components[j].restitutionCoefficient = originalRestitution[j];
            components[j].staticFrictionCoefficient = originalStaticFriction[j];
            components[j].dynamicFrictionCoefficient = originalDynamicFriction[j];
        }
    }
};
dvec3 Physics::gravity = { 0,0,-9.81 };
double Physics::deltaT = 1.0 / 70.0;
int Physics::subStepCount = 8;
double Physics::restitutionMult = 1.0;
double Physics::dynamicFrictionMult = 1.0;
double Physics::staticFrictionMult = 1.0;