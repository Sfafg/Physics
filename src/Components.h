#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Mesh.h"
#include "VG/VG.h"
#include "ECS.h"

/// @brief Komponent przechowujący dane do transformacji obiektów.
struct Transform : public ECS::Component<Transform>
{
    glm::dvec3 position;
    glm::dvec3 scale;
    glm::dquat rotation;
    bool updateFlag;

    Transform(const glm::dvec3& position = glm::dvec3(0, 0, 0), const glm::dvec3& scale = glm::dvec3(1, 1, 1), const glm::dquat& rotation = glm::dquat(1, 0, 0, 0));

    /// @brief Oblicza macierz transformacji
    /// @return macierz transformacji
    glm::mat4 Matrix() const;

    /// @brief Oblicza wektor  {0,1,0} w koordynatach globalnych.
    /// @return wektor
    glm::dvec3 Forward() const;

    /// @brief Oblicza wektor  {0,0,1} w koordynatach globalnych.
    /// @return wektor
    glm::dvec3 Up() const;

    /// @brief Oblicza wektor  {1,0,0} w koordynatach globalnych.
    /// @return wektor
    glm::dvec3 Right() const;
};

/// @brief Komponent przechowujący informacje na temat siatki oraz materiału pewnego obiektu.
struct MeshArray : public ECS::Component<MeshArray>
{
    uint8_t materialID;
    uint8_t materialVariantID;
    uint16_t meshID;

    MeshArray() {}
    MeshArray(uint8_t materialID, uint8_t materialVariantID, uint16_t meshID)
        :materialID(materialID), materialVariantID(materialVariantID), meshID(meshID)
    {}
};

/// @brief Komponent przechowujący dane obiektu kolizyjnego.
struct Collider : public ECS::Component<Collider>
{
    enum class Type
    {
        Sphere, Box
    } type;
    union
    {
        double radius;
        glm::dvec3 size;
    };
    double boundingSphereRadius;

    Collider();
    Collider(double radius);
    Collider(const glm::dvec3& boxSize);

    glm::dvec3 SupportMapping(glm::dvec3 direction) const;

private:
    double GetBoundingSphereRadius() const;
    glm::dvec3 LocalSupportMapping(glm::dvec3 direction) const;
};

/// @brief Komponent przechowujacy dane ciała sztywnego.
struct RigidBody : public ECS::Component<RigidBody>
{
    glm::dvec3 velocity;
    glm::dvec3 angularVelocity;
    glm::dvec3 centerOfMass;

    glm::dvec3 previousPosition;
    glm::dvec3 previousVelocity;
    glm::dvec3 previousAngularVelocity;
    glm::dquat previousRotation;

    glm::dmat3 inertiaTensor;
    glm::dmat3 inverseInertiaTensor;

    double mass;
    double inverseMass;
    double restitutionCoefficient;
    double staticFrictionCoefficient;
    double dynamicFrictionCoefficient;

    RigidBody();
    RigidBody(const glm::dvec3& velocity, const glm::dvec3& angularVelocity, const glm::dvec3& centerOfMass, double mass, const glm::dmat3& inertiaTensor, double restitutionCoefficient = 1.0, double staticFrictionCoefficient = 0.9, double dynamicFrictionCoefficient = 0.68);

    glm::dvec3& GetPosition();
    glm::dquat& GetRotation();
    double GetMass(const glm::dvec3& point, const glm::dvec3& normal) const;

    void AddForce(const glm::dvec3& force, double deltaT);
    void ApplyPositionalImpulse(const glm::dvec3& impulse, const glm::dvec3& point);
    void ApplyVelocityImpulse(const glm::dvec3& impulse, const glm::dvec3& point);

private:
    void Integrate(double deltaT);
    void UpdateVelocities(double deltaT);

    friend class Physics;
};

ENTITY(Transform, MeshArray, Collider, RigidBody);