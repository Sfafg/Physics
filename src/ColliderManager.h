#pragma once
#include "Components.h"

using dvec2 = glm::dvec2;
using dvec3 = glm::dvec3;
using dquat = glm::dquat;

class PenetrationConstraint
{
public:
    RigidBody& a;
    RigidBody& b;
    dvec3 pointA;
    dvec3 pointB;
    dvec3 normal;
    double depth;
private:
    double normalLambda;

public:
    PenetrationConstraint() = delete;
    PenetrationConstraint(RigidBody& a, RigidBody& b, dvec3 pointA, dvec3 pointB, dvec3 normal, double depth);

    dvec3 GetR1() const;
    dvec3 GetR2() const;

    void SolvePositions(double deltaT);
    void SolveVelocities(double restitutionCutoff, double deltaT);
};

class ColliderManager : public ECS::System<Collider>
{
public:
    static bool GJK(const Collider& a, const Collider& b, dvec3* simplex);
    static void EPA(const Collider& a, const Collider& b, std::vector<dvec3>& polytope, dvec3* normal, double* depth);
    static std::tuple<dvec3, dvec3> Clipping(const Collider& a, const Collider& b, const dvec3& normal, double depth);
    static std::vector<PenetrationConstraint> GetPenetrations();
    static std::vector<PenetrationConstraint> GetPenetrations(Collider* collider, const std::vector<Collider*>& colliders);
    static std::vector<Collider*> QuerryInRadius(const dvec3& position, double radius);

private:
    static uint16_t Hash(glm::vec<3, uint16_t> position);

    static void DrawSupportDifferance(const Collider& a, const Collider& b);
    static dvec3 SupportMapping(const Collider& collider, dvec3 direction);
    static dvec3 Support(const Collider& a, const Collider& b, const dvec3& direction);
    static std::vector<dvec3> SupportMappingArray(const Collider& collider, dvec3 direction);
};