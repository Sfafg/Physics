#include "Constraints.h"

std::tuple<dvec3, double> ApplyPositionDelta(dvec3 changeNormal, double changeMagnitude, RigidBody& a, dvec3 r1, RigidBody& b, dvec3 r2, double alpha)
{
    double lambda = -changeMagnitude / (a.GetMass(r1, changeNormal) + b.GetMass(r1, changeNormal) + alpha);
    dvec3 impulse = lambda * changeNormal;

    return { impulse,lambda };
}

PenetrationConstraint::PenetrationConstraint() {}
PenetrationConstraint::PenetrationConstraint(RigidBody& a, RigidBody& b, dvec3 pointA, dvec3 pointB, dvec3 normal, double depth)
    :a(&a), b(&b), pointA(pointA), pointB(pointB), normal(normal), depth(depth)
{}

dvec3 PenetrationConstraint::GetR1() const
{
    return a->GetRotation() * pointA;
}

dvec3 PenetrationConstraint::GetR2() const
{
    return b->GetRotation() * pointB;
}

void PenetrationConstraint::SolvePositions(double deltaT)
{
    // Points relative to centers of mass.
    dvec3 r1 = GetR1();
    dvec3 r2 = GetR2();
    depth = glm::dot(r1 + a->GetComponent<Transform>().position - (r2 + b->GetComponent<Transform>().position), normal);

    if (depth <= 0) return;

    // Handle depenetration.
    auto [normalImpulse, normalLambda] = ApplyPositionDelta(normal, depth, *a, r1, *b, r2, 0);
    a->ApplyPositionalImpulse(normalImpulse, r1);
    b->ApplyPositionalImpulse(-normalImpulse, r2);

    r1 = GetR1();
    r2 = GetR2();

    // Handle static friction.
    dvec3 P1 = r1 + a->GetComponent<Transform>().position;
    dvec3 P2 = r2 + b->GetComponent<Transform>().position;
    dvec3 P1_ = a->previousPosition + a->previousRotation * pointA;
    dvec3 P2_ = b->previousPosition + b->previousRotation * pointB;
    dvec3 deltaP = (P1 - P1_) - (P2 - P2_);
    dvec3 deltaPTangential = deltaP - glm::dot(deltaP, normal) * normal;
    double length = glm::length(deltaPTangential);
    if (length != 0)
        deltaPTangential /= length;

    static double sum = 0;
    static int count = 0;
    sum += normalLambda;
    count++;
    auto [tangentialImpulse, tangentialLambda] = ApplyPositionDelta(deltaPTangential, length, *a, r1, *b, r2, 0);
    double frictionCeofficient = (a->staticFrictionCoefficient + b->staticFrictionCoefficient) * 0.5;
    if (std::abs(tangentialLambda) < frictionCeofficient * std::abs(normalLambda))
    {
        a->ApplyPositionalImpulse(tangentialImpulse, r1);
        b->ApplyPositionalImpulse(-tangentialImpulse, r2);
    }
}

void PenetrationConstraint::SolveVelocities(double restitutionCutoff, double deltaT)
{
    if (depth <= 0) return;

    Transform& aTr = a->GetComponent<Transform>();
    Transform& bTr = b->GetComponent<Transform>();
    dvec3 r1 = GetR1();
    dvec3 r2 = GetR2();
    dvec3 velocity = (a->velocity + glm::cross(a->angularVelocity, r1)) - (b->velocity + glm::cross(b->angularVelocity, r2));
    double normalVelocity = glm::dot(normal, velocity);

    {
        dvec3 tangentialVelocity = velocity - normal * normalVelocity;
        double tangentialSpeed = glm::length(tangentialVelocity);
        if (tangentialSpeed != 0)
            tangentialVelocity /= tangentialSpeed;
        double frictionCoefficient = (a->dynamicFrictionCoefficient + b->dynamicFrictionCoefficient) * 0.5;
        double w1 = a->inverseMass + glm::dot(glm::cross(aTr.rotation * r1, tangentialVelocity), a->inverseInertiaTensor * glm::cross(aTr.rotation * r1, tangentialVelocity));
        double w2 = b->inverseMass + glm::dot(glm::cross(r2, tangentialVelocity), b->inverseInertiaTensor * glm::cross(r2, tangentialVelocity));
        dvec3 frictionDeltaV = -tangentialVelocity * std::min(frictionCoefficient * std::abs(normalLambda / (deltaT * deltaT)), tangentialSpeed);
        dvec3 p = frictionDeltaV / (a->GetMass(r1, tangentialVelocity) + b->GetMass(r2, tangentialVelocity));
        a->ApplyVelocityImpulse(p, r1);
        b->ApplyVelocityImpulse(-p, r2);
    }

    // Handle restitution.
    double restitution = (a->restitutionCoefficient + b->restitutionCoefficient) * 0.5;
    if (glm::abs(normalVelocity) <= 2 * restitutionCutoff * deltaT)
        restitution = 0.0;

    dvec3 previousVelocity = (a->previousVelocity + glm::cross(a->previousAngularVelocity, r1)) - (b->previousVelocity + glm::cross(b->previousAngularVelocity, r2));
    double previousNormalVelocity = glm::dot(normal, previousVelocity);
    dvec3 deltaV = normal * (-normalVelocity + std::min(-restitution * previousNormalVelocity, 0.0));
    double w1 = a->inverseMass + glm::dot(glm::cross(aTr.rotation * r1, normal), a->inverseInertiaTensor * glm::cross(aTr.rotation * r1, normal));
    double w2 = b->inverseMass + glm::dot(glm::cross(r2, normal), b->inverseInertiaTensor * glm::cross(r2, normal));
    dvec3 p = deltaV / (w1 + w2);

    a->ApplyVelocityImpulse(p, r1);
    b->ApplyVelocityImpulse(-p, r2);
}