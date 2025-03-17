#include "ColliderManager.h"
#include <math.h>
#include <unordered_set>


bool IsVectorZero(dvec3 v, double e)
{
    v = glm::abs(v);
    return v.x < e && v.y < e && v.z < e;
}

bool ColliderManager::GJK(const Collider& a, const Collider& b, Support* simplex)
{
    // Wyznacz p1 punkt który na pewno jest wewnątrz różnicy minkowskiego, jest to np różnica środków obiektów.
    dvec3 n = { 0,0,1 };
    simplex[0] = Support(a.GetComponent<Transform>().position, b.GetComponent<Transform>().position);

    // Znjadź p2 na różnicy minkowskiego w kierunku punktu (0,0) z punktu p1.
    if (!IsVectorZero(simplex[0], 0.000000001))
        n = glm::normalize(-(dvec3) simplex[0]);
    simplex[1] = Support(a, b, n);

    // Jeżeli p2 nie jest za punktem (0,0) to znaczy że nie ma kolizji.
    double dotProduct = glm::dot((dvec3) simplex[1], n);
    if (dotProduct < 0)
        return false;

    // Znjadź p3 na różnicy minkowskiego w kierunku punktu (0,0) z punktu p2.
    n = glm::normalize(-(dvec3) simplex[1]);
    simplex[2] = Support(a, b, n);

    // Jeżeli p1, p2, p3 są kolniowe, to znajdź nowy p3 w kierunku prostopadłym do p1, p2.
    dvec3 cross = glm::cross((dvec3) simplex[1] - (dvec3) simplex[0], (dvec3) simplex[2] - (dvec3) simplex[0]);
    if (IsVectorZero(cross, 0.000000001))
    {
        n = glm::normalize(glm::cross(-(dvec3) simplex[0], -(dvec3) simplex[1] + dvec3{ ((dvec3) simplex[1]).y,  ((dvec3) simplex[1]).z,  ((dvec3) simplex[1]).x }));
        simplex[2] = Support(a, b, n);
    }

    // Jeżeli p3 nie jest za punktem (0,0) to znaczy że nie ma kolizji.
    dotProduct = glm::dot((dvec3) simplex[2], n);
    if (dotProduct < 0)
        return false;

    // Znajdź punkt p4, w kierunku przeciwnym do normalnej trójkąta stworzonego z p1, p2, p3.
    // Jeżeli normalna jest skierowana w kierunku (0,0) to odwróć trójkąt
    n = glm::cross((dvec3) simplex[1] - (dvec3) simplex[0], (dvec3) simplex[2] - (dvec3) simplex[0]);
    if (glm::dot(n, (dvec3) simplex[0]) > 0)
    {
        std::swap(simplex[0], simplex[1]);
        n = glm::cross((dvec3) simplex[1] - (dvec3) simplex[0], (dvec3) simplex[2] - (dvec3) simplex[0]);
    }
    n = glm::normalize(n);
    simplex[3] = Support(a, b, n);
    dotProduct = glm::dot((dvec3) simplex[3], n);

    // Jeżeli p4 nie jest za punktem (0,0) to nie ma kolizji, natomiast jeżeli jest za to jest kolizja.
    if (dotProduct <= 0)
        return false;

    // Simplex jest już tetrahedronem, ma 4 trójkąty jeden zawsze na pewno ma normalną po dobrej stronie,
    // przejdź po pozostałych 3, jeżeli (0.0) jest po stronie normalnej trójkąta, to znaczy że jest poza tetrahedronem z tej strony,
    // znajdź nowy punkt na różnicy minkowskiego w kierunku normalnej trójkąta, i zamień go z punktem, który jest na przeciwko trójkąta.
    // Tak powstają 3 nowe trójkąty, powtórz aż żaden nie zostanie zmieniony, czyli jest kolizja, 
    // lub aż nie znajdziesz punktu, który jest za (0,0) czyli nie ma kolizji. 
    int lastCorrectedPoint = 2;
    for (int i = 0; i < 64; i++)
    {
        bool triangleOrder = (lastCorrectedPoint % 2) == 0;
        int j = 0;
        for (; j < 3; j++)
        {
            int baseIndex = lastCorrectedPoint;
            int secondIndex = (baseIndex + j + 1) % 4;
            int triangleWinding = (j + (triangleOrder ? -1 : 1)) % 3;
            if (triangleWinding < 0) triangleWinding = 3 + triangleWinding;
            int thirdIndex = (baseIndex + triangleWinding + 1) % 4;

            n = glm::cross((dvec3) simplex[secondIndex] - (dvec3) simplex[baseIndex], (dvec3) simplex[thirdIndex] - (dvec3) simplex[baseIndex]);
            if (glm::dot(n, (dvec3) simplex[baseIndex]) < 0)
            {
                int unusedIndex = 6 - (baseIndex + secondIndex + thirdIndex);
                std::swap(simplex[secondIndex], simplex[thirdIndex]);

                n = glm::normalize(n);
                simplex[unusedIndex] = Support(a, b, n);

                lastCorrectedPoint = unusedIndex;
                if (glm::dot(n, (dvec3) simplex[unusedIndex]) < 0)
                    return false;

                j = -1;
                break;
            }
        }
        if (j == 3)
            return true;
    }

    return false;
}

struct EPATriangle
{
    dvec3 normal;
    double distance;
    short indices[3];

    EPATriangle(Support* polytope, const short(&indices)[3])
        : indices{ indices[0],indices[1],indices[2] }
    {
        dvec3 a = polytope[indices[0]];
        dvec3 b = polytope[indices[1]];
        dvec3 c = polytope[indices[2]];
        dvec3 u = a - b;
        dvec3 v = a - c;

        if (IsVectorZero(u, 0.000000001))
            u = dvec3(0.0000001, 0.0, 0.0);

        normal = glm::cross(u, v);
        if (IsVectorZero(normal, 0.000000001))
            normal = glm::cross(glm::cross(u, a), u);

        normal = glm::normalize(normal);
        distance = abs(glm::dot(normal, a));
    }

    bool operator<(const EPATriangle& rhs) const
    {
        return distance < rhs.distance;
    }

    dvec3 GetBarycentricCoordinates(Support* polytope, dvec3 p)
    {
        dvec3 r1 = polytope[indices[0]];
        dvec3 r2 = polytope[indices[1]];
        dvec3 r3 = polytope[indices[2]];

        double t = glm::dot(glm::cross(r1 - r3, r2 - r3), normal);

        dvec3 barycentricCoordinates;
        if (-std::numeric_limits<double>::epsilon() <= t && t <= std::numeric_limits<double>::epsilon())
        {
            barycentricCoordinates.x = 0;
            barycentricCoordinates.y = 0;
        }
        else
        {
            barycentricCoordinates.x = glm::dot(glm::cross(p - r3, r2 - r3), normal) / t;
            barycentricCoordinates.y = glm::dot(glm::cross(p - r3, r3 - r1), normal) / t;
        }

        barycentricCoordinates.z = 1 - barycentricCoordinates.x - barycentricCoordinates.y;

        return barycentricCoordinates;
    }
};

void ColliderManager::EPA(const Collider& a, const Collider& b, std::vector<Support>& polytope, dvec3* normal, double* depth, dvec3* p1, dvec3* p2)
{
    // Na początku politop składa się z 4 punktów,
    // dodaj wszystkie trójkaty tetrahedronu do listy.
    std::vector<EPATriangle> triangles;
    triangles.push_back(EPATriangle(polytope.data(), { 0, 2, 1 }));
    triangles.push_back(EPATriangle(polytope.data(), { 1, 3, 0 }));
    triangles.push_back(EPATriangle(polytope.data(), { 2, 0, 3 }));
    triangles.push_back(EPATriangle(polytope.data(), { 3, 1, 2 }));

    EPATriangle* closestTriangle = nullptr;
    for (int i = 0; i < 64; i++)
    {
        closestTriangle = &std::min_element(triangles.begin(), triangles.end())[0];

        // Znajdź nowy punkt na różnicy minkowskiego,
        // Jeżeli jest dostatecznie blisko akutalnego punktu,
        // to przyjmujemy, że jest na powieszchni orginalnego krztałtu i kończymi pętle.
        Support newSupport = Support(a, b, closestTriangle->normal);
        double newDistance = glm::dot(closestTriangle->normal, (dvec3) newSupport);
        if (newDistance - closestTriangle->distance <= 0.00001)
            break;

        polytope.push_back(newSupport);

        // Zapewnij że politop jest wypukły,
        // poprzez usunięcie wszystkich punktów widocznych z nowego punktu.
        std::set<std::tuple<short, short>> uniqueEdges;
        for (int j = triangles.size() - 1; j >= 0; j--)
        {
            // Czy trójkąt jest widoczny z nowego punktu.
            auto& entry = triangles[j];
            if (glm::dot(entry.normal, (dvec3) polytope[entry.indices[0]]) - glm::dot(entry.normal, (dvec3) newSupport) <= std::numeric_limits<double>::epsilon())
            {
                for (int k = 0; k < 3; k++)
                {
                    short aIndex = triangles[j].indices[k];
                    short bIndex = triangles[j].indices[(k + 1) % 3];
                    if (uniqueEdges.contains({ bIndex,aIndex }))
                        uniqueEdges.erase({ bIndex,aIndex });
                    else
                        uniqueEdges.insert({ aIndex,bIndex });
                }

                triangles[j] = triangles[triangles.size() - 1];
                triangles.pop_back();
            }
        }
        // Połącz wszystkie wolne krawędzie z nowym punktem.
        for (auto [edge1, edge2] : uniqueEdges)
            triangles.push_back(EPATriangle(polytope.data(), { edge1,edge2,short(polytope.size() - 1) }));
    }

    if (closestTriangle)
    {
        *normal = -closestTriangle->normal;
        *depth = closestTriangle->distance;

        dvec3 p = closestTriangle->distance * closestTriangle->normal;
        dvec3 barycentricCoordinates = closestTriangle->GetBarycentricCoordinates(polytope.data(), p);
        *p1 =
            barycentricCoordinates.x * polytope[closestTriangle->indices[0]].GetA() +
            barycentricCoordinates.y * polytope[closestTriangle->indices[1]].GetA() +
            barycentricCoordinates.z * polytope[closestTriangle->indices[2]].GetA();
        *p2 =
            barycentricCoordinates.x * polytope[closestTriangle->indices[0]].GetB() +
            barycentricCoordinates.y * polytope[closestTriangle->indices[1]].GetB() +
            barycentricCoordinates.z * polytope[closestTriangle->indices[2]].GetB();
    }
}

bool ColliderManager::GetPenetration(const Collider& a, const Collider& b, PenetrationConstraint* penetration)
{
    if (a.GetComponent<RigidBody>().inverseMass == 0 && b.GetComponent<RigidBody>().inverseMass == 0)
        return false;

    auto&& aTr = a.GetComponent<Transform>();
    auto&& bTr = b.GetComponent<Transform>();

    dvec3 normal = bTr.position - aTr.position;
    double sqrDist = glm::dot(normal, normal);

    if (sqrDist >= pow(a.boundingSphereRadius + b.boundingSphereRadius, 2))
    {
        return false;
    }

    if (a.type == Collider::Type::Sphere && b.type == Collider::Type::Sphere)
    {
        double dist = sqrt(sqrDist);
        double depth = (a.radius + b.radius) - dist;
        normal /= dist;

        dvec3 p1 = normal * a.radius;
        dvec3 p2 = -normal * b.radius;

        p1 = glm::inverse(aTr.rotation) * p1;
        p2 = glm::inverse(bTr.rotation) * p2;

        *penetration = PenetrationConstraint(a.GetComponent<RigidBody>(), b.GetComponent<RigidBody>(), p1, p2, normal, depth);
        return true;
    }

    Support simplex[4];
    if (GJK(a, b, simplex))
    {
        dvec3 normal;
        double depth;
        dvec3 p1, p2;
        std::vector<Support> polytope;
        polytope.assign(&simplex[0], &simplex[0] + 4);
        EPA(a, b, polytope, &normal, &depth, &p1, &p2);

        p1 = dquat(glm::inverse(aTr.rotation)) * (p1 - dvec3(aTr.position));
        p2 = dquat(glm::inverse(bTr.rotation)) * (p2 - dvec3(bTr.position));

        *penetration = PenetrationConstraint(a.GetComponent<RigidBody>(), b.GetComponent<RigidBody>(), p1, p2, normal, depth);
        return true;
    }

    return false;
}
std::vector<PenetrationConstraint> ColliderManager::GetPenetrations()
{
    std::vector<PenetrationConstraint> penetrations;
    for (int i = 0; i < components.size(); i++)
    {
        for (int j = i + 1; j < components.size(); j++)
        {
            PenetrationConstraint penetration;
            if (GetPenetration(components[i], components[j], &penetration))
                penetrations.emplace_back(std::move(penetration));
        }
    }

    return penetrations;
}

std::vector<PenetrationConstraint> ColliderManager::GetPenetrations(Collider* collider, const std::vector<Collider*>& colliders)
{
    std::unordered_set<CollisionPair, CollisionPair::Hash> collisionPairs;
    std::vector<PenetrationConstraint> penetrations;
    for (int j = 0; j < colliders.size(); j++)
    {
        if (colliders[j] == collider || collisionPairs.contains({ colliders[j], collider }))
            continue;

        PenetrationConstraint penetration;
        if (GetPenetration(*collider, *colliders[j], &penetration))
        {
            penetrations.emplace_back(std::move(penetration));
            collisionPairs.insert({ colliders[j], collider });
        }
    }

    return penetrations;
}

void ColliderManager::QuarryInRadius(Collider& a, double radiusMultiplier, std::unordered_set<CollisionPair, CollisionPair::Hash>* pairs)
{
    double radius = a.boundingSphereRadius * radiusMultiplier;
    dvec3 position = a.GetComponent<Transform>().position;
    for (auto&& i : components)
    {
        if (&a == &i)
            continue;

        if (glm::dot(i.GetComponent<Transform>().position - position, i.GetComponent<Transform>().position - position) >= pow(radius + i.boundingSphereRadius, 2))
            continue;

        if (pairs->contains(CollisionPair{ &a,&i }) || pairs->contains(CollisionPair{ &i, &a }))
            continue;

        pairs->insert(CollisionPair{ &a,&i });
    }
}