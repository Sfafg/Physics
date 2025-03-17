#include "Components.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

Transform::Transform(const glm::dvec3& position, const glm::dvec3& scale, const glm::dquat& rotation)
    :position(position), scale(scale), rotation(rotation)
{}

glm::mat4 Transform::Matrix() const
{
    return glm::translate(glm::mat4(1), glm::vec3(position)) * glm::mat4(glm::quat(rotation)) * glm::scale(glm::mat4(1), glm::vec3(scale));
}

glm::dvec3 Transform::Forward() const
{
    auto v = rotation * glm::dvec4(0, 1, 0, 0);
    return v;
}

glm::dvec3 Transform::Up() const
{
    auto v = rotation * glm::dvec4(0, 0, 1, 0);
    return v;
}

glm::dvec3 Transform::Right() const
{
    auto v = rotation * glm::dvec4(1, 0, 0, 0);
    return v;
}

MeshArray::MeshArray(std::vector<Mesh>&& meshes) : meshes(meshes) {}


Collider::Collider() {}

Collider::Collider(double radius) :type(Type::Sphere), radius(radius), boundingSphereRadius(GetBoundingSphereRadius()) {}

Collider::Collider(const glm::dvec3& boxSize) :type(Type::Box), size(boxSize), boundingSphereRadius(GetBoundingSphereRadius()) {}

glm::dvec3 Collider::SupportMapping(glm::dvec3 direction) const
{
    auto& transform = GetComponent<Transform>();
    direction = glm::inverse(transform.rotation) * direction;
    return transform.rotation * LocalSupportMapping(direction) + transform.position;
}

double Collider::GetBoundingSphereRadius() const
{
    switch (type)
    {
    case Collider::Type::Sphere:
        return radius;

    case Collider::Type::Box:
        return sqrt(size.x * size.x + size.y * size.y + size.z * size.z);

    default:
        return -1;
    }
}

glm::dvec3 Collider::LocalSupportMapping(glm::dvec3 direction) const
{
    switch (type)
    {
    case Collider::Type::Sphere:
        return glm::normalize(direction) * radius;

    case Collider::Type::Box:
        return glm::dvec3{ direction.x > 0 ? 1 : -1, direction.y > 0 ? 1 : -1, direction.z > 0 ? 1 : -1 } *size;

    default:
        return { 0, 0, 0 };
    }
}

glm::dmat3 GetInertiaTensor(glm::dvec3 scale, Collider::Type type)
{
    switch (type)
    {
    case Collider::Type::Sphere:
        return glm::dmat3(2.0 / 5.0 * pow(std::max({ scale.x,scale.y,scale.z }), 2.0));

    case Collider::Type::Box:
        scale *= 2;
        return glm::dmat3(
            1.0 / 12.0 * (scale.x * scale.x + scale.z * scale.z), 0, 0,
            0, 1.0 / 12.0 * (scale.y * scale.y + scale.z * scale.z), 0,
            0, 0, 1.0 / 12.0 * (scale.x * scale.x + scale.y * scale.y));

    default:return glm::dmat3(1.0);
    }
}

RigidBody::RigidBody() {}

RigidBody::RigidBody(const glm::dvec3& velocity, const glm::dvec3& angularVelocity, const glm::dvec3& centerOfMass, double mass, const glm::dmat3& inertiaTensor, double restitutionCoefficient, double staticFrictionCoefficient, double dynamicFrictionCoefficient)
    :velocity(velocity), angularVelocity(angularVelocity), centerOfMass(centerOfMass),
    inertiaTensor(inertiaTensor* mass), inverseInertiaTensor(glm::inverse(inertiaTensor* mass)),
    mass(mass), inverseMass(1.0f / mass), restitutionCoefficient(restitutionCoefficient),
    staticFrictionCoefficient(staticFrictionCoefficient), dynamicFrictionCoefficient(dynamicFrictionCoefficient)
{
    if (mass == std::numeric_limits<decltype(mass)>::infinity())
    {
        inverseInertiaTensor = glm::dmat3({ 0,0,0,0,0,0,0,0,0 });
        inverseMass = 0;
    }
}

glm::dvec3& RigidBody::GetPosition()
{
    return GetComponent<Transform>().position;
}

glm::dquat& RigidBody::GetRotation()
{
    return GetComponent<Transform>().rotation;
}

double RigidBody::GetMass(const glm::dvec3& point, const glm::dvec3& normal) const
{
    return inverseMass + glm::dot(glm::cross(point, normal), inverseInertiaTensor * glm::cross(point, normal));
}

void RigidBody::AddForce(const glm::dvec3& force, double deltaT)
{
    if (inverseMass == 0)
        return;

    velocity += force * deltaT;
}

void RigidBody::ApplyPositionalImpulse(const glm::dvec3& impulse, const glm::dvec3& point)
{
    GetPosition() += impulse * inverseMass;
    GetRotation() += 0.5 * glm::dquat(0, inverseInertiaTensor * glm::cross(point, impulse)) * GetRotation();
}

void RigidBody::ApplyVelocityImpulse(const glm::dvec3& impulse, const glm::dvec3& point)
{
    velocity += impulse * inverseMass;
    angularVelocity += inverseInertiaTensor * glm::cross(point, impulse);
}

void RigidBody::Integrate(double deltaT)
{
    previousPosition = GetPosition();
    previousRotation = GetRotation();
    previousVelocity = velocity;
    previousAngularVelocity = angularVelocity;

    if (inverseMass == 0)
        return;

    angularVelocity += deltaT * inverseInertiaTensor * -glm::cross(angularVelocity, inertiaTensor * angularVelocity);

    GetPosition() += velocity * deltaT;
    GetRotation() += deltaT * 0.5 * glm::dquat(0, angularVelocity) * GetRotation();
    GetRotation() = glm::normalize(GetRotation());
}

void RigidBody::UpdateVelocities(double deltaT)
{
    velocity = (GetPosition() - previousPosition) / deltaT;

    glm::dquat rotationDelta = GetRotation() * glm::inverse(previousRotation);
    angularVelocity = 2.0 * glm::dvec3(rotationDelta.x, rotationDelta.y, rotationDelta.z) / deltaT;
    angularVelocity = rotationDelta.w >= 0 ? angularVelocity : -angularVelocity;
}
