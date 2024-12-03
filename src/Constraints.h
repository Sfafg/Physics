#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Components.h"
using dvec2 = glm::dvec2;
using dvec3 = glm::dvec3;
using dquat = glm::dquat;

class PenetrationConstraint
{
public:
    RigidBody* a;
    RigidBody* b;
    dvec3 pointA;
    dvec3 pointB;
    dvec3 normal;
    double depth;

private:
    double normalLambda;

public:
    PenetrationConstraint();
    PenetrationConstraint(RigidBody& a, RigidBody& b, dvec3 pointA, dvec3 pointB, dvec3 normal, double depth);

    void SolvePositions(double deltaT);
    void SolveVelocities(double restitutionCutoff, double deltaT);

private:
    dvec3 GetR1() const;
    dvec3 GetR2() const;
};