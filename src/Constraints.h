#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Components.h"
using dvec2 = glm::dvec2;
using dvec3 = glm::dvec3;
using dquat = glm::dquat;

/// @brief Ogarnicznik penetracji, używany przy odpowiedzi na kolizje.
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

    /// @brief Rozwiązuje pozycje obiektów biorących udział.
    /// @param deltaT zmiana w czasie
    void SolvePositions(double deltaT);

    /// @brief Rozwiązuje prędkości obiektów biorących udział.
    /// @param restitutionCutoff granica prędkości normalnej poniżej której restytucja jest ustawiana na 0
    /// @param deltaT zmiana w czasie
    void SolveVelocities(double restitutionCutoff, double deltaT);

private:
    dvec3 GetR1() const;
    dvec3 GetR2() const;
};