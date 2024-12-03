#pragma once
#include "Components.h"
#include "Constraints.h"
#include <unordered_set>

using dvec2 = glm::dvec2;
using dvec3 = glm::dvec3;
using dquat = glm::dquat;

struct CollisionPair
{
    const Collider* a;
    const Collider* b;

    CollisionPair() : a(nullptr), b(nullptr) {}
    CollisionPair(const Collider* a, const Collider* b) :a(a), b(b) {}

    bool operator==(const CollisionPair& rhs) const
    {
        return a == rhs.a && b == rhs.b;
    }

    struct Hash
    {
        size_t operator ()(const CollisionPair& o) const { return (size_t) o.a * 1203881 + (size_t) o.b; }
    };
};

class Support
{
    dvec3 a;
    dvec3 b;

public:
    Support() {}
    Support(dvec3 a, dvec3 b) :a(a), b(b) {}
    Support(const Collider& a, const Collider& b, dvec3 n) : a(a.SupportMapping(-n)), b(b.SupportMapping(n)) {}

    operator dvec3() { return b - a; }
    dvec3& GetA() { return a; }
    const dvec3& GetA() const { return a; }
    dvec3& GetB() { return b; }
    const dvec3& GetB() const { return b; }
};

class ColliderManager : public ECS::System<Collider>
{
public:
    static bool GJK(const Collider& a, const Collider& b, Support* simplex);
    static void EPA(const Collider& a, const Collider& b, std::vector<Support>& polytope, dvec3* normal, double* depth, dvec3* p1, dvec3* p2);
    static bool GetPenetration(const Collider& a, const Collider& b, PenetrationConstraint* penetration);
    static std::vector<PenetrationConstraint> GetPenetrations();
    static std::vector<PenetrationConstraint> GetPenetrations(Collider* collider, const std::vector<Collider*>& colliders);
    static void QuerryInRadius(Collider& a, double radiusMultiplier, std::unordered_set<CollisionPair, CollisionPair::Hash>* pairs);
};