#include <cmath>
#include <iostream>
#include <limits>

#include <glm/gtc/matrix_transform.hpp>

#include "object.hpp"

namespace hw4 {
    SphereObject::SphereObject(
        float radius,
        glm::vec3 center,
        const std::shared_ptr<Material>& material
    )
        : Object(
            BoundingBox(glm::vec3(-1), glm::vec3(1)),
            glm::scale(glm::translate(glm::mat4(1.0), center), glm::vec3(radius)),
            material
        ) {}

    boost::optional<Intersection> SphereObject::find_intersection(const Ray& r) const {
        float b = glm::dot(2.0f * r.direction(), r.origin());
        float c = glm::dot(r.origin(), r.origin()) - 1;

        float qterm = b * b - 4 * c;

        if (qterm < 0)
            return boost::none;

        float t = -(b + std::sqrt(qterm)) / 2;

        if (t <= 0)
            return boost::none;

        auto p = r.origin() + t * r.direction();

        return Intersection(
            p,
            p,
            glm::vec2(
                0.5 + std::atan2(p.z, p.x) / tau,
                0.5 + std::asin(p.y) * 2 / tau
            ),
            this->material(),
            t
        );
    }

    boost::optional<Intersection> TriMesh::find_intersection(const Ray& r, const Triangle& tri) const {
        const auto& a = this->m_vertices[tri.a];
        const auto& b = this->m_vertices[tri.b];
        const auto& c = this->m_vertices[tri.c];

        // Use the Möller-Trumbore algorithm for intersection
        auto ab = b.pos - a.pos;
        auto ac = c.pos - a.pos;

        auto pvec = glm::cross(r.direction(), ac);
        float det = glm::dot(ab, pvec);

        // If det is negative (or close to being negative), then the triangle is backfacing. So we
        // bail early in that case.
        if (det <= 0.00001f) return boost::none;

        float inv_det = 1.0 / det;

        auto tvec = r.origin() - a.pos;
        float u = glm::dot(tvec, pvec) * inv_det;

        if (u < 0 || u > 1) return boost::none;

        auto qvec = glm::cross(tvec, ab);
        float v = glm::dot(r.direction(), qvec) * inv_det;

        if (v < 0 || u + v > 1) return boost::none;

        float t = glm::dot(ac, qvec) * inv_det;

        if (t < 0) return boost::none;

        // Now that we know an intersection occurs, we need to use u and v to perform barycentric
        // interpolation to find the correct attribute values
        return Intersection(
            r.origin() + t * r.direction(),
            glm::normalize((1 - u - v) * a.normal + u * b.normal + v * c.normal),
            (1 - u - v) * a.texcoord + u * b.texcoord + v * c.texcoord,
            nullptr,
            t
        );
    }

    boost::optional<Intersection> TriMesh::find_intersection(const Ray& r) const {
        float depth = std::numeric_limits<float>::infinity();
        boost::optional<Intersection> intersection;

        this->m_bvh.search(r, [&](auto& n) {
            for (Triangle* t : n.objects) {
                auto new_intersection = this->find_intersection(r, *t);

                if (new_intersection && new_intersection->distance() < depth) {
                    depth = new_intersection->distance();
                    intersection = new_intersection;
                }
            }
        });

        return intersection;
    }

    boost::optional<Intersection> TriMeshObject::find_intersection(const Ray& r) const {
        auto intersection = this->m_mesh->find_intersection(r);

        if (intersection) {
            intersection->material(this->material());
        }

        return intersection;
    }
}
