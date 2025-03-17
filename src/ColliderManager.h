#pragma once
#include "Components.h"
#include "Constraints.h"
#include <unordered_set>

using dvec2 = glm::dvec2;
using dvec3 = glm::dvec3;
using dquat = glm::dquat;

/// @brief Struktura reprezentująca parę obiektów w kolizji.
struct CollisionPair
{
    const Collider* a;
    const Collider* b;

    CollisionPair() : a(nullptr), b(nullptr) {}
    CollisionPair(const Collider* a, const Collider* b) :a(a), b(b) {}

    CollisionPair Inverse() const { return CollisionPair(b, a); }

    bool operator==(const CollisionPair& rhs) const
    {
        return a == rhs.a && b == rhs.b;
    }

    /// @brief Struktura odpowiedzialna za wyznaczanie hasha pary kolizji, potrzebne do std::unordered_set
    struct Hash
    {
        size_t operator()(const CollisionPair& o) const
        {
            return (size_t) o.a * 1203881 + (size_t) o.b;
        }
    };
};

/// @brief Klasa odpowiedzialna za tworzenie i przechowywanie punktów w różnicy Minkowskiego.
/// @details Przechowuje dane jako punkt z obiektu A oraz punkt z obiektu B.
/// Punkty na różnicy Minkowskiego obliczane są na bierząco przez b - a.
class Support
{
    /// @brief Punkt z obiektu A
    dvec3 a;
    /// @brief Punkt z obiektu B
    dvec3 b;

public:
    Support() {}

    /// @brief Konstruktor przyjmujący punkt z obiektów A i B.
    /// @param a punkt z obiektu A
    /// @param b punkt z obiektu B
    Support(dvec3 a, dvec3 b) :a(a), b(b) {}

    /// @brief Konstruktor wyznaczający różnice Minkowskiego dwóch obiektów, w kierunku \p n
    /// @param a obiekt A
    /// @param b obiekt B
    /// @param n kierunek
    Support(const Collider& a, const Collider& b, dvec3 n) : a(a.SupportMapping(-n)), b(b.SupportMapping(n)) {}

    /// @brief Operator zwracający konkretny punkt na w różnicy Minkowskiego
    operator dvec3() { return b - a; }

    dvec3& GetA() { return a; }
    const dvec3& GetA() const { return a; }
    dvec3& GetB() { return b; }
    const dvec3& GetB() const { return b; }
};

/// @brief System zarządzania obiektami kolizji.
/// @details Przechowuje obiekty kolizji oraz udostępnia funkcje do wyznaczania parametrów kolizji między obiektami,
/// oraz wyznaczania obiektów w pewnej okolicy.
class ColliderManager : public ECS::System<Collider>
{
public:
    /// @brief Implementacja algorytmu Gilbert'a-Johnson'a-Keerthi'ego.
    /// @details Stwierdza czy zachodzi kolizja między dwoma obiektami na podstawie,
    /// funkcji wspomagajacej tworząc simpleks (czyli w 3D tetrahedron), który jeżeli zawiera
    /// początek układu współrzędnych to znaczy, że jest kolizja.
    /// @see Support
    /// @param a obiekt A
    /// @param b obiekt B
    /// @param simplex Zwracany simplex, musi mieć miejsce na conajmniej 4 elementy
    /// @return Prawda jeżeli zachodzi kolizja, fałsz jeżeli nie ma kolizji
    static bool GJK(const Collider& a, const Collider& b, Support* simplex);

    /// @brief Implementacja algorytmu Expanding Polytope Algorythm,
    /// @details Działa na simpleksie z algorytmu GJK, oblicza normalną, głębokość oraz punkty kolizji.
    /// Działa przez dodawanie punktów z funkcji wspomagającej w kierunku normalnej trójkąta, który jest najbliżej początku układu współrzędnych
    /// i łączenie istniejących punktów w trójkąty, zachowując wypukłość utworzonego politopu.
    /// @see Support
    /// @see GJK
    /// @param a obiekt A
    /// @param b obietk B
    /// @param polytope politop, początkowo muszą się wewnątrz znajdywać punkty z wstępnego simpleksa z funkcji GJK
    /// @param normal wyjściowa normalna kolizji w kierunku z obiektu B do obiektu A
    /// @param depth wyjściowa głębokość kolizji
    /// @param p1 wyjsciowy punkt kolizji na obiekcie A
    /// @param p2 wyjsciowy punkt kolizji na obiekcie B
    static void EPA(const Collider& a, const Collider& b, std::vector<Support>& polytope, dvec3* normal, double* depth, dvec3* p1, dvec3* p2);

    /// @brief Funkcja zwracająca ograniczk penetracji jeżeli zachodzi kolizja, potrzebny w rozwiązywaniu kolizji.
    /// @param a obiekt A
    /// @param b obiekt B
    /// @param penetration wyjściowy ogranicznik penetracji 
    /// @return Prawda jeżeli zachodzi kolizja, fałsz jeżeli nie ma kolizji
    static bool GetPenetration(const Collider& a, const Collider& b, PenetrationConstraint* penetration);
    static bool AreColliding(const Collider& a, const Collider& b)
    {
        Support simplex[4];
        return GJK(a, b, simplex);
    }

    /// @brief Funkcja zwracająca ograniczniki penetracji dla wszystkich obiektów w kolizji.
    /// @see GetPenetration
    /// @return lista ograniczników penetracji
    static std::vector<PenetrationConstraint> GetPenetrations();

    /// @brief Funkcja zwracająca ograniczniki penetracji dla kolizji między @p collider oraz @p colliders .
    /// @param collider obiekt A
    /// @param colliders lista obiektów B
    /// @return lista ograniczników penetracji
    static std::vector<PenetrationConstraint> GetPenetrations(Collider* collider, const std::vector<Collider*>& colliders);

    /// @brief Funkcja zwracająca listę unikalnych par kolizji w odległości @p radiusMultiplier * średnica kuli opisanej na obiektcie @p a od pozycji obiektu @p a 
    /// @param a obiekt A
    /// @param radiusMultiplier mnożnik średnicy kuli opisanej na obiekcie @p a 
    /// @param pairs wyjściowa lista unikalnych par kolizji, takich że pierwszy obiekt w parze to obiekt A
    static void QuarryInRadius(Collider& a, double radiusMultiplier, std::unordered_set<CollisionPair, CollisionPair::Hash>* pairs);
};