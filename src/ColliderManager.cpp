#include "ColliderManager.h"
#include "DebugDraw.h"
#include <math.h>
#include <unordered_set>

bool IsVectorZero(dvec3 v, double e)
{
    v = glm::abs(v);
    return v.x < e && v.y < e && v.z < e;
}

void DrawSimplex(dvec3* simplex, int count)
{
    DebugDraw::color.a = 0.01;
    DebugDraw::Triangle(simplex[1], simplex[0], simplex[2]);
    DebugDraw::Triangle(simplex[0], simplex[1], simplex[3]);
    DebugDraw::Triangle(simplex[2], simplex[0], simplex[3]);
    DebugDraw::Triangle(simplex[1], simplex[2], simplex[3]);

    DebugDraw::color.a = 1;
    DebugDraw::Line(simplex[0], simplex[1], 0.001);
    DebugDraw::Line(simplex[0], simplex[2], 0.001);
    DebugDraw::Line(simplex[1], simplex[2], 0.001);
    DebugDraw::Line(simplex[0], simplex[3], 0.001);
    DebugDraw::Line(simplex[1], simplex[3], 0.001);
    DebugDraw::Line(simplex[2], simplex[3], 0.001);
}

bool ColliderManager::GJK(const Collider& a, const Collider& b, dvec3* simplex)
{
    // Wyznacz p1 punkt który na pewno jest wewnątrz różnicy minkowskiego, jest to np różnica środków obiektów.
    dvec3 n = { 0,0,1 };
    simplex[0] = b.GetComponent<Transform>().position - a.GetComponent<Transform>().position;
    // DebugDraw::Sphere(simplex[0], 0.03);

    // Znjadź p2 na różnicy minkowskiego w kierunku punktu (0,0) z punktu p1.
    if (!IsVectorZero(simplex[0], 0.000000001))
        n = glm::normalize(-simplex[0]);
    simplex[1] = Support(a, b, n);
    // DebugDraw::Ray(simplex[0], n, 0.03);
    // DebugDraw::Sphere(simplex[1], 0.03);

    // Jeżeli p2 nie jest za punktem (0,0) to znaczy że nie ma kolizji.
    double dotProduct = glm::dot(simplex[1], n);
    if (dotProduct < 0)
        return false;

    // Znjadź p3 na różnicy minkowskiego w kierunku punktu (0,0) z punktu p2.
    n = glm::normalize(-simplex[1]);
    simplex[2] = Support(a, b, n);

    // Jeżeli p1, p2, p3 są kolniowe, to znajdź nowy p3 w kierunku prostopadłym do p1, p2.
    dvec3 cross = glm::cross(simplex[1] - simplex[0], simplex[2] - simplex[0]);
    if (IsVectorZero(cross, 0.000000001))
    {
        n = glm::normalize(glm::cross(-simplex[0], -simplex[1] + dvec3{ simplex[1].y, simplex[1].z, simplex[1].x }));
        simplex[2] = Support(a, b, n);
    }
    // DebugDraw::Ray(simplex[1], n, 0.03);
    // DebugDraw::Sphere(simplex[2], 0.03);

    // Jeżeli p3 nie jest za punktem (0,0) to znaczy że nie ma kolizji.
    dotProduct = glm::dot(simplex[2], n);
    if (dotProduct < 0)
        return false;

    // Znajdź punkt p4, w kierunku przeciwnym do normalnej trójkąta stworzonego z p1, p2, p3.
    // Jeżeli normalna jest skierowana w kierunku (0,0) to odwróć trójkąt
    n = glm::cross(simplex[1] - simplex[0], simplex[2] - simplex[0]);
    if (glm::dot(n, simplex[0]) > 0)
    {
        std::swap(simplex[0], simplex[1]);
        n = glm::cross(simplex[1] - simplex[0], simplex[2] - simplex[0]);
    }
    n = glm::normalize(n);
    simplex[3] = Support(a, b, n);
    dotProduct = glm::dot(simplex[3], n);
    // DebugDraw::Ray((simplex[0] + simplex[1] + simplex[2]) / 3.0, n, 0.03);
    // DebugDraw::Sphere(simplex[3], 0.03);

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

            n = glm::cross(simplex[secondIndex] - simplex[baseIndex], simplex[thirdIndex] - simplex[baseIndex]);
            if (glm::dot(n, simplex[baseIndex]) < 0)
            {
                int unusedIndex = 6 - (baseIndex + secondIndex + thirdIndex);
                std::swap(simplex[secondIndex], simplex[thirdIndex]);
                n = glm::normalize(n);
                simplex[unusedIndex] = Support(a, b, n);

                lastCorrectedPoint = unusedIndex;
                // DebugDraw::color = { 0,0,1,1 };
                // DebugDraw::Sphere({ 0,0,0 }, 0.02);
                // DebugDraw::color = { 1,0,0,1 };
                // DebugDraw::Ray((simplex[secondIndex] + simplex[baseIndex] + simplex[thirdIndex]) / 3.0, n, 0.0075);
                // DebugDraw::Sphere(simplex[unusedIndex], 0.03);
                // DebugDraw::color = glm::vec4{ 1.0, 1.0, 0.0, 1.0 };
                // DrawSimplex(simplex, 4);
                // DebugDraw::matrix = glm::translate(DebugDraw::matrix, { 0,0,2 });
                if (glm::dot(n, simplex[unusedIndex]) < 0)
                    return false;

                j = -1;
                break;
            }
        }
        if (j == 3)
        {
            // DebugDraw::matrix = glm::translate(DebugDraw::matrix, { 0,0,2 });
            // DrawSupportDifferance(a, b);
            // DebugDraw::color = { 0,0,1,1 };
            // DebugDraw::Sphere({ 0,0,0 }, 0.02);
            // DebugDraw::color = glm::vec4{ 0, 1.0, 0.0, 1.0 };
            // DrawSimplex(simplex, 4);
            return true;
        }
    }

    return false;
}

struct EPATriangle
{
    dvec3 normal;
    double distance;
    short indices[3];

    EPATriangle(dvec3* polytope, const short(&indices)[3])
        : indices{ indices[0],indices[1],indices[2] }
    {
        dvec3 a = polytope[indices[0]];
        dvec3 b = polytope[indices[1]];
        dvec3 c = polytope[indices[2]];
        dvec3 u = a - b;
        dvec3 v = a - c;

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
};

void ColliderManager::EPA(const Collider& a, const Collider& b, std::vector<dvec3>& polytope, dvec3* normal, double* depth)
{
    // Na początku politop składa się z 4 punktów,
    // dodaj wszystkie trójkaty tetrahedronu do listy.
    std::vector<EPATriangle> triangles;
    triangles.push_back(EPATriangle(polytope.data(), { 0, 2, 1 }));
    triangles.push_back(EPATriangle(polytope.data(), { 1, 3, 0 }));
    triangles.push_back(EPATriangle(polytope.data(), { 2, 0, 3 }));
    triangles.push_back(EPATriangle(polytope.data(), { 3, 1, 2 }));

    DebugDraw::matrix = glm::mat4(1);
    EPATriangle* closestTriangle = nullptr;
    static int count = -1;
    for (int i = 0; i < 64; i++)
    {
        closestTriangle = &std::min_element(triangles.begin(), triangles.end())[0];

        // Znajdź nowy punkt na różnicy minkowskiego,
        // Jeżeli jest dostatecznie blisko akutalnego punktu,
        // to przyjmujemy, że jest na powieszchni orginalnego krztałtu i kończymi pętle.
        dvec3 newSupport = Support(a, b, closestTriangle->normal);

        DebugDraw::color = { 0,0,1,1 };
        if (count == 0)
            DebugDraw::Sphere({ 0,0,0 }, 0.011, 1000);
        for (auto&& k : triangles)
        {
            if (count == 0)
            {
                if (&k == closestTriangle)
                    continue;

                DebugDraw::color = { 1,1,0,0.04 };
                DebugDraw::Triangle(polytope[k.indices[0]], polytope[k.indices[1]], polytope[k.indices[2]], 1000);
                DebugDraw::color.a = 1;
                DebugDraw::Line(polytope[k.indices[0]], polytope[k.indices[1]], 0.001, 1000);
                DebugDraw::Line(polytope[k.indices[0]], polytope[k.indices[2]], 0.001, 1000);
                DebugDraw::Line(polytope[k.indices[1]], polytope[k.indices[2]], 0.001, 1000);
                // DebugDraw::Ray((polytope[k.indices[0]] + polytope[k.indices[1]] + polytope[k.indices[2]]) / 3.0, k.normal * k.distance, 0.001, 1000);
            }
        }
        if (count == 0)
        {
            DebugDraw::color = { 1,0,0,0.04 };
            DebugDraw::Triangle(polytope[closestTriangle->indices[0]], polytope[closestTriangle->indices[1]], polytope[closestTriangle->indices[2]], 1000);
            DebugDraw::color.a = 1;
            DebugDraw::Sphere(newSupport, 0.01, 1000);
            DebugDraw::Line(polytope[closestTriangle->indices[0]], polytope[closestTriangle->indices[1]], 0.0011, 1000);
            DebugDraw::Line(polytope[closestTriangle->indices[0]], polytope[closestTriangle->indices[2]], 0.0011, 1000);
            DebugDraw::Line(polytope[closestTriangle->indices[1]], polytope[closestTriangle->indices[2]], 0.0011, 1000);
            // DebugDraw::Ray((polytope[closestTriangle->indices[0]] + polytope[closestTriangle->indices[1]] + polytope[closestTriangle->indices[2]]) / 3.0, closestTriangle->normal * closestTriangle->distance, 0.0011, 1000);
        }
        DebugDraw::matrix = glm::translate(DebugDraw::matrix, { 0,0,2 });

        double newDistance = glm::dot(closestTriangle->normal, newSupport);
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
            if (glm::dot(entry.normal, polytope[entry.indices[0]]) - glm::dot(entry.normal, newSupport) <= std::numeric_limits<double>::epsilon())
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
    // count++;
    if (count == 1000)
        count = 0;

    if (closestTriangle)
    {
        *normal = -closestTriangle->normal;
        *depth = closestTriangle->distance;
    }
}

bool LinePlaneIntersection(dvec3 planePoint, dvec3 planeNormal, dvec3 planeTangent, double planeWidth, dvec3 linePoint, dvec3 lineDirection, double lineLength, dvec3* point)
{
    double dot = glm::dot(lineDirection, planeNormal);
    if (dot == 0) return false;
    double d = glm::dot(planePoint - linePoint, planeNormal) / dot;
    if (d < 0) return false;

    *point = linePoint + lineDirection * d;
    double tangentDist = glm::dot(*point - planePoint, planeTangent);
    if (tangentDist < 0 || tangentDist > planeWidth) return false;

    return d <= lineLength;
}

std::tuple<dvec3, dvec3> ColliderManager::Clipping(const Collider& a, const Collider& b, const dvec3& normal, double depth)
{
    std::vector<dvec3> pointsA = SupportMappingArray(a, normal);
    std::vector<dvec3> pointsB = SupportMappingArray(b, -normal);

    if (pointsA.size() == 1)
        return { pointsA[0], pointsA[0] - normal * depth };
    if (pointsB.size() == 1)
        return { pointsB[0] + normal * depth , pointsB[0] };

    int insideCount = 0;
    dvec3 middle = { 0,0,0 };
    std::vector<bool> isPointAInside(pointsA.size());
    std::vector<bool> isPointBInside(pointsB.size());
    for (int i = 0; i < pointsA.size(); i++) isPointAInside[i] = true;
    for (int i = 0; i < pointsB.size(); i++) isPointBInside[i] = true;
    for (int i = 0; i < pointsA.size(); i++)
    {
        dvec3 planeTangent = pointsA[(i + 1) % pointsA.size()] - pointsA[i];
        double planeWidth = glm::length(planeTangent);
        planeTangent /= planeWidth;
        dvec3 planeNormal = -glm::cross(planeTangent, normal);
        dvec3 planePoint = pointsA[i];

        for (int j = 0; j < pointsB.size(); j++)
        {
            dvec3 lineNormal = pointsB[(j + 1) % pointsB.size()] - pointsB[j];
            double lineLength = glm::length(lineNormal);
            lineNormal /= lineLength;
            dvec3 lineTangent = glm::cross(lineNormal, normal);
            dvec3 linePoint = pointsB[j];

            if (pointsB.size() == 2 || (isPointAInside[i] && glm::dot(pointsA[i] - linePoint, lineTangent) <= 0))
                isPointAInside[i] = false;
            if (pointsA.size() == 2 || (isPointBInside[j] && glm::dot(pointsB[j] - planePoint, planeNormal) <= 0))
                isPointBInside[j] = false;

            if (j == 1 && pointsB.size() == 2)
                break;

            dvec3 intersectionPoint;
            if (LinePlaneIntersection(planePoint, planeNormal, planeTangent, planeWidth, linePoint, lineNormal, lineLength, &intersectionPoint))
            {
                middle += intersectionPoint;
                insideCount++;
            }
        }
    }
    for (int i = 0; i < pointsA.size(); i++)
    {
        if (isPointAInside[i])
        {
            middle += pointsA[i];
            insideCount++;
        }
    }
    for (int i = 0; i < pointsB.size(); i++)
    {
        if (isPointBInside[i])
        {
            middle += pointsB[i];
            insideCount++;
        }
    }
    if (insideCount != 0)
        middle /= double(insideCount);
    else
    {
        for (int i = 0; i < pointsA.size(); i++)
            middle += pointsA[i];

        middle /= pointsA.size();
    }

    return { middle + normal * depth, middle };

}

std::vector<PenetrationConstraint> ColliderManager::GetPenetrations()
{
    std::vector<PenetrationConstraint> penetrations;
    for (int i = 0; i < components.size(); i++)
    {
        for (int j = i + 1; j < components.size(); j++)
        {
            auto&& a = components[i];
            auto&& b = components[j];
            auto& aTr = a.GetComponent<Transform>();
            auto& bTr = b.GetComponent<Transform>();

            if (a.GetComponent<RigidBody>().inverseMass == 0 && b.GetComponent<RigidBody>().inverseMass == 0)
                continue;

            if (a.type == Collider::Type::Sphere && b.type == Collider::Type::Sphere)
            {
                dvec3 normal = bTr.position - aTr.position;
                double sqrDist = glm::dot(normal, normal);

                if (sqrDist < pow(a.radius + b.radius, 2))
                {
                    double dist = sqrt(sqrDist);
                    double depth = (a.radius + b.radius) - dist;
                    normal /= dist;

                    dvec3 p1 = normal * a.radius;
                    dvec3 p2 = -normal * b.radius;

                    p1 = glm::inverse(aTr.rotation) * p1;
                    p2 = glm::inverse(bTr.rotation) * p2;

                    penetrations.push_back(
                        PenetrationConstraint(a.GetComponent<RigidBody>(), b.GetComponent<RigidBody>(), p1, p2, normal, depth)
                    );
                }
                continue;
            }

            dvec3 simplex[4] = { {DBL_MAX,0,0},{DBL_MAX,0,0} ,{DBL_MAX,0,0} ,{DBL_MAX,0,0} };
            if (GJK(a, b, simplex))
            {
                dvec3 normal;
                double depth;
                std::vector<dvec3> polytope;
                polytope.assign(&simplex[0], &simplex[0] + 4);
                EPA(a, b, polytope, &normal, &depth);

                auto [p1, p2] = Clipping(a, b, normal, depth);

                p1 = dquat(glm::inverse(aTr.rotation)) * (p1 - dvec3(aTr.position));
                p2 = dquat(glm::inverse(bTr.rotation)) * (p2 - dvec3(bTr.position));

                penetrations.push_back(
                    PenetrationConstraint(a.GetComponent<RigidBody>(), b.GetComponent<RigidBody>(), p1, p2, normal, depth)
                );
            }
        }
    }

    return penetrations;
}

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
        size_t operator ()(const CollisionPair& o) const { return (size_t) o.a + (size_t) o.b; }
    };
};
std::vector<PenetrationConstraint> ColliderManager::GetPenetrations(Collider* collider, const std::vector<Collider*>& colliders)
{
    std::unordered_set<CollisionPair, CollisionPair::Hash> collisionPairs;
    std::vector<PenetrationConstraint> penetrations;
    auto&& b = *collider;
    auto& bTr = b.GetComponent<Transform>();
    for (int j = 0; j < colliders.size(); j++)
    {
        if (colliders[j] == collider || collisionPairs.contains({ colliders[j], collider }))
            continue;

        auto&& a = *colliders[j];
        auto& aTr = a.GetComponent<Transform>();

        if (a.GetComponent<RigidBody>().inverseMass == 0 && b.GetComponent<RigidBody>().inverseMass == 0)
            continue;

        if (a.type == Collider::Type::Sphere && b.type == Collider::Type::Sphere)
        {
            dvec3 normal = bTr.position - aTr.position;
            double sqrDist = glm::dot(normal, normal);

            if (sqrDist < pow(a.radius + b.radius, 2))
            {
                double dist = sqrt(sqrDist);
                double depth = (a.radius + b.radius) - dist;
                normal /= dist;

                dvec3 p1 = normal * a.radius;
                dvec3 p2 = -normal * b.radius;

                p1 = glm::inverse(aTr.rotation) * p1;
                p2 = glm::inverse(bTr.rotation) * p2;

                penetrations.push_back(
                    PenetrationConstraint(a.GetComponent<RigidBody>(), b.GetComponent<RigidBody>(), p1, p2, normal, depth)
                );
            }

            collisionPairs.insert({ colliders[j], collider });
            continue;
        }

        dvec3 simplex[4] = { {DBL_MAX,0,0},{DBL_MAX,0,0} ,{DBL_MAX,0,0} ,{DBL_MAX,0,0} };
        if (GJK(a, b, simplex))
        {
            dvec3 normal;
            double depth;
            std::vector<dvec3> polytope;
            polytope.assign(&simplex[0], &simplex[0] + 4);
            EPA(a, b, polytope, &normal, &depth);

            auto [p1, p2] = Clipping(a, b, normal, depth);

            p1 = dquat(glm::inverse(aTr.rotation)) * (p1 - dvec3(aTr.position));
            p2 = dquat(glm::inverse(bTr.rotation)) * (p2 - dvec3(bTr.position));

            penetrations.push_back(
                PenetrationConstraint(a.GetComponent<RigidBody>(), b.GetComponent<RigidBody>(), p1, p2, normal, depth)
            );
            collisionPairs.insert({ colliders[j], collider });
        }
    }

    return penetrations;
}

std::vector<Collider*> ColliderManager::QuerryInRadius(const dvec3& position, double radius)
{
    std::vector<Collider*> result;

    for (auto&& i : components)
    {
        dvec3 dir = i.GetComponent<Transform>().position - position;
        if (glm::dot(dir, dir) < pow(radius + i.boundingSphereRadius, 2))
            result.push_back(&i);
    }

    return result;
}

uint16_t ColliderManager::Hash(glm::vec<3, uint16_t> position)
{
    return (position.x * 92837111) ^ (position.y * 689287499) ^ (position.z * 283923481);
}

void ColliderManager::DrawSupportDifferance(const Collider& a, const Collider& b)
{
    const int resolution = 16;
    const float PI = 3.14159265359f;

    static std::vector<Entity> entities;
    if (entities.size() != (resolution + 1) * (resolution * 2))
    {
        entities.resize((resolution + 1) * (resolution * 2));
        for (int i = 0; i < entities.size(); i++)
        {
            entities[i] = Entity::AddEntity(
                Transform({ 0,0,0 }),
                MeshArray({ DebugDraw::sphereMesh })
            );
        }
    }

    for (int inclination = 0; inclination <= resolution; inclination++)
    {
        for (int azimuth = 0; azimuth < resolution * 2; azimuth++)
        {
            float alfa = inclination / (float) resolution * PI;
            float beta = azimuth / (float) resolution * PI;

            glm::vec3 dir;
            dir.x = sin(alfa) * cos(beta);
            dir.y = sin(alfa) * sin(beta);
            dir.z = cos(alfa);

            glm::vec3 point = SupportMapping(b, dir) - SupportMapping(a, -dir);
            int i = inclination * resolution * 2 + azimuth;
            entities[i].GetComponent<Transform>().position = point;
            entities[i].GetComponent<Transform>().scale = { 0.01f ,0.01f ,0.01f };
        }
    }
}

dvec3 LocalSupportMapping(const Collider& collider, dvec3 direction)
{
    switch (collider.type)
    {
    case Collider::Type::Sphere:
        return glm::normalize(direction) * collider.radius;

    case Collider::Type::Box:
        return dvec3{ direction.x > 0 ? 1 : -1, direction.y > 0 ? 1 : -1, direction.z > 0 ? 1 : -1 } *collider.size;

    default:
        return { 0, 0, 0 };
    }
}

dvec3 ColliderManager::SupportMapping(const Collider& collider, dvec3 direction)
{
    auto& transform = collider.GetComponent<Transform>();
    direction = glm::inverse(transform.rotation) * direction;
    return transform.rotation * LocalSupportMapping(collider, direction) + transform.position;
}

dvec3 ColliderManager::Support(const Collider& a, const Collider& b, const dvec3& direction)
{
    return SupportMapping(b, direction) - SupportMapping(a, -direction);
}

std::vector<dvec3> LocalSupportMappingArray(const Collider& collider, const dvec3& direction)
{
    std::vector<dvec3> points;
    switch (collider.type)
    {
    case Collider::Type::Sphere:
        points.push_back(direction * collider.radius);
        break;

    case Collider::Type::Box:
    {
        // Get which of direction's x,y,z are 0
        int nonZeroCount = 0;
        int nonZeroIndices[3]{};
        for (int i = 0; i < 3; i++)
            if (abs(direction[i]) >= 0.0001)
            {
                nonZeroIndices[nonZeroCount] = i;
                nonZeroCount++;
            }
        // If one of x,y,z are non-zero then return a plane
        if (nonZeroCount == 1)
        {
            points.reserve(4);

            int index = nonZeroIndices[0];
            dvec3 point;
            int dirSign = direction[index] < 0 ? -1 : 1;
            point[index] = dirSign;

            point[(index + 1) % 3] = -1;
            point[(index + 2) % 3] = -1;
            points.push_back(point * collider.size);

            point[(index + 1) % 3] = 1;
            point[(index + 2) % 3] = -1;
            points.push_back(point * collider.size);

            point[(index + 1) % 3] = 1;
            point[(index + 2) % 3] = 1;
            points.push_back(point * collider.size);

            point[(index + 1) % 3] = -1;
            point[(index + 2) % 3] = 1;
            points.push_back(point * collider.size);

            if (dirSign == -1)
            {
                std::swap(points[0], points[3]);
                std::swap(points[1], points[2]);
            }
        }
        // If 2 of x,y,z are non zero return a line
        else if (nonZeroCount == 2)
        {
            points.reserve(2);

            int index = (nonZeroIndices[0] + 1) % 3;
            if (index == nonZeroIndices[1])
                index = (nonZeroIndices[1] + 1) % 3;

            dvec3 point;
            point[nonZeroIndices[0]] = direction[nonZeroIndices[0]] < 0 ? -1 : 1;
            point[nonZeroIndices[1]] = direction[nonZeroIndices[1]] < 0 ? -1 : 1;

            point[index] = -1;
            points.push_back(point * collider.size);

            point[index] = 1;
            points.push_back(point * collider.size);
        }
        else if (nonZeroCount == 3)
        {
            dvec3 point;
            point.x = direction.x < 0 ? -1 : 1;
            point.y = direction.y < 0 ? -1 : 1;
            point.z = direction.z < 0 ? -1 : 1;

            points.push_back(point * collider.size);
        }
    }
    break;

    default: break;
    }

    return points;
}

std::vector<dvec3> ColliderManager::SupportMappingArray(const Collider& collider, dvec3 direction)
{
    auto& transform = collider.GetComponent<Transform>();
    direction = glm::inverse(transform.rotation) * direction;
    std::vector<dvec3> mapping = LocalSupportMappingArray(collider, direction);
    for (auto& i : mapping)
        i = transform.rotation * i + transform.position;

    return std::move(mapping);
}

std::tuple<dvec3, double> ApplyPositionDelta(dvec3 changeNormal, double changeMagnitude, RigidBody& a, dvec3 r1, RigidBody& b, dvec3 r2, double alpha)
{
    double lambda = -changeMagnitude / (a.GetMass(r1, changeNormal) + b.GetMass(r1, changeNormal) + alpha);
    dvec3 impulse = lambda * changeNormal;

    return { impulse,lambda };
}

PenetrationConstraint::PenetrationConstraint(RigidBody& a, RigidBody& b, dvec3 pointA, dvec3 pointB, dvec3 normal, double depth)
    :a(a), b(b), pointA(pointA), pointB(pointB), normal(normal), depth(depth)
{}

dvec3 PenetrationConstraint::GetR1() const
{
    return a.GetRotation() * pointA;
}

dvec3 PenetrationConstraint::GetR2() const
{
    return b.GetRotation() * pointB;
}

void PenetrationConstraint::SolvePositions(double deltaT)
{
    // Points relative to centers of mass.
    dvec3 r1 = GetR1();
    dvec3 r2 = GetR2();
    depth = glm::dot(r1 + a.GetComponent<Transform>().position - (r2 + b.GetComponent<Transform>().position), normal);

    if (depth <= 0) return;

    // Handle depenetration.
    auto [normalImpulse, normalLambda] = ApplyPositionDelta(normal, depth, a, r1, b, r2, 0);
    a.ApplyPositionalImpulse(normalImpulse, r1);
    b.ApplyPositionalImpulse(-normalImpulse, r2);

    r1 = GetR1();
    r2 = GetR2();

    // Handle static friction.
    dvec3 P1 = r1 + a.GetComponent<Transform>().position;
    dvec3 P2 = r2 + b.GetComponent<Transform>().position;
    dvec3 P1_ = a.previousPosition + a.previousRotation * pointA;
    dvec3 P2_ = b.previousPosition + b.previousRotation * pointB;
    dvec3 deltaP = (P1 - P1_) - (P2 - P2_);
    dvec3 deltaPTangential = deltaP - glm::dot(deltaP, normal) * normal;
    double length = glm::length(deltaPTangential);
    if (length != 0)
        deltaPTangential /= length;

    static double sum = 0;
    static int count = 0;
    sum += normalLambda;
    count++;
    auto [tangentialImpulse, tangentialLambda] = ApplyPositionDelta(deltaPTangential, length, a, r1, b, r2, 0);
    double frictionCeofficient = (a.staticFrictionCoefficient + b.staticFrictionCoefficient) * 0.5;
    if (std::abs(tangentialLambda) < frictionCeofficient * std::abs(normalLambda))
    {
        a.ApplyPositionalImpulse(tangentialImpulse, r1);
        b.ApplyPositionalImpulse(-tangentialImpulse, r2);
    }
}

void PenetrationConstraint::SolveVelocities(double restitutionCutoff, double deltaT)
{
    if (depth <= 0) return;

    Transform& aTr = a.GetComponent<Transform>();
    Transform& bTr = b.GetComponent<Transform>();
    dvec3 r1 = GetR1();
    dvec3 r2 = GetR2();
    dvec3 velocity = (a.velocity + glm::cross(a.angularVelocity, r1)) - (b.velocity + glm::cross(b.angularVelocity, r2));
    double normalVelocity = glm::dot(normal, velocity);

    {
        dvec3 tangentialVelocity = velocity - normal * normalVelocity;
        double tangentialSpeed = glm::length(tangentialVelocity);
        if (tangentialSpeed != 0)
            tangentialVelocity /= tangentialSpeed;
        double frictionCoefficient = (a.dynamicFrictionCoefficient + b.dynamicFrictionCoefficient) * 0.5;
        double w1 = a.inverseMass + glm::dot(glm::cross(aTr.rotation * r1, tangentialVelocity), a.inverseInertiaTensor * glm::cross(aTr.rotation * r1, tangentialVelocity));
        double w2 = b.inverseMass + glm::dot(glm::cross(r2, tangentialVelocity), b.inverseInertiaTensor * glm::cross(r2, tangentialVelocity));
        dvec3 frictionDeltaV = -tangentialVelocity * std::min(frictionCoefficient * std::abs(normalLambda / (deltaT * deltaT)), tangentialSpeed);
        dvec3 p = frictionDeltaV / (a.GetMass(r1, tangentialVelocity) + b.GetMass(r2, tangentialVelocity));
        a.ApplyVelocityImpulse(p, r1);
        b.ApplyVelocityImpulse(-p, r2);
    }

    // Handle restitution.
    double restitution = (a.restitutionCoefficient + b.restitutionCoefficient) * 0.5;
    if (glm::abs(normalVelocity) <= 2 * restitutionCutoff * deltaT)
        restitution = 0.0;

    dvec3 previousVelocity = (a.previousVelocity + glm::cross(a.previousAngularVelocity, r1)) - (b.previousVelocity + glm::cross(b.previousAngularVelocity, r2));
    double previousNormalVelocity = glm::dot(normal, previousVelocity);
    dvec3 deltaV = normal * (-normalVelocity + std::min(-restitution * previousNormalVelocity, 0.0));
    double w1 = a.inverseMass + glm::dot(glm::cross(aTr.rotation * r1, normal), a.inverseInertiaTensor * glm::cross(aTr.rotation * r1, normal));
    double w2 = b.inverseMass + glm::dot(glm::cross(r2, normal), b.inverseInertiaTensor * glm::cross(r2, normal));
    dvec3 p = deltaV / (w1 + w2);

    a.ApplyVelocityImpulse(p, r1);
    b.ApplyVelocityImpulse(-p, r2);
}