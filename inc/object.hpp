#ifndef HW4_OBJECT_HPP
#define HW4_OBJECT_HPP

#include <array>
#include <memory>

#include <boost/optional.hpp>
#include <glm/glm.hpp>

#include "bvh.hpp"
#include "material.hpp"
#include "ray.hpp"

namespace hw4 {
    constexpr float tau = 6.2831853071f;

    class Intersection {
        glm::vec3 m_point;
        glm::vec3 m_normal;
        glm::vec2 m_texcoord;

        std::shared_ptr<Material> m_material;

        float m_distance;
    public:
        Intersection() : m_distance(0) {}
        Intersection(
            glm::vec3 point,
            glm::vec3 normal,
            glm::vec2 texcoord,
            std::shared_ptr<Material> material,
            float distance
        )
            : m_point(point), m_normal(normal), m_texcoord(texcoord),
              m_material(std::move(material)), m_distance(distance) {}

        const glm::vec3& point() const { return this->m_point; }
        const glm::vec3& normal() const { return this->m_normal; }
        const glm::vec2& texcoord() const { return this->m_texcoord; }
        float distance() const { return this->m_distance; }

        PointMaterial material() const { return this->m_material->at_point(this->m_texcoord); }
        Intersection& material(std::shared_ptr<Material> material) {
            this->m_material = material;
            return *this;
        }

        Intersection transform(const glm::mat4& transform, float dist_mult) const {
            return Intersection(
                glm::vec3(transform * glm::vec4(this->m_point, 1)),
                // Since we only allow orthogonal transformations and translations, we can just use
                // the transform directly.
                glm::normalize(glm::mat3(transform) * this->m_normal),
                this->m_texcoord,
                this->m_material,
                this->m_distance * dist_mult
            );
        }
    };

    class Object {
        glm::mat4 m_transform;
        glm::mat4 m_inv_transform;

        BoundingBox m_obb;
        BoundingBox m_aabb;

        std::shared_ptr<Material> m_material;
    public:
        Object(
            BoundingBox obb,
            const glm::mat4& transform,
            const std::shared_ptr<Material>& material
        )
            : m_transform(transform), m_inv_transform(glm::inverse(transform)), m_obb(obb),
              m_aabb(transform * obb), m_material(material) {}
        virtual ~Object() = default;

        const glm::mat4& transform() const { return this->m_transform; }
        Object& transform(const glm::mat4& transform) {
            this->m_transform = transform;
            this->m_inv_transform = glm::inverse(this->m_transform);
            this->m_aabb = transform * this->m_obb;

            return *this;
        }

        const glm::mat4& inv_transform() const { return this->m_inv_transform; }

        const BoundingBox& obb() const { return this->m_obb; }
        const BoundingBox& aabb() const { return this->m_aabb; }

        const std::shared_ptr<Material>& material() const { return this->m_material; }

        virtual boost::optional<Intersection> find_intersection(const Ray& r) const = 0;
    };

    class SphereObject : public Object {
    public:
        SphereObject(
            float radius,
            glm::vec3 center,
            const std::shared_ptr<Material>& material
        );

        virtual boost::optional<Intersection> find_intersection(const Ray& r) const;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texcoord;
    };

    struct Triangle {
        unsigned int a;
        unsigned int b;
        unsigned int c;
    };

    class TriMesh {
        std::vector<Vertex> m_vertices;
        std::vector<Triangle> m_triangles;
        BVH<Triangle> m_bvh;

        BoundingBox calc_triangle_bounding_box(const Triangle& t) {
            const auto& a = this->m_vertices[t.a].pos;
            const auto& b = this->m_vertices[t.b].pos;
            const auto& c = this->m_vertices[t.c].pos;

            return BoundingBox(
                glm::vec3(
                    std::min(a.x, std::min(b.x, c.x)),
                    std::min(a.y, std::min(b.y, c.y)),
                    std::min(a.z, std::min(b.z, c.z))
                ),
                glm::vec3(
                    std::max(a.x, std::max(b.x, c.x)),
                    std::max(a.y, std::max(b.y, c.y)),
                    std::max(a.z, std::max(b.z, c.z))
                )
            );
        }
    public:
        TriMesh(std::vector<Vertex> vertices, std::vector<Triangle> triangles)
            : m_vertices(std::move(vertices)), m_triangles(std::move(triangles)),
              m_bvh(BVH<Triangle>::construct(
                  ([this]() {
                      std::vector<Triangle*> ts;

                      for (auto& t : this->m_triangles) {
                          ts.push_back(&t);
                      }

                      return std::move(ts);
                  })(),
                  [this](const auto& t) { return this->calc_triangle_bounding_box(t); }
              )) {}

        const BoundingBox& obb() const { return this->m_bvh.root()->box; }

        const std::vector<Vertex>& vertices() const { return this->m_vertices; }
        const std::vector<Triangle>& triangles() const { return this->m_triangles; }
        const BVH<Triangle>& bvh() const { return this->m_bvh; }

        boost::optional<Intersection> find_intersection(const Ray& r, const Triangle& t) const;
        boost::optional<Intersection> find_intersection(const Ray& r) const;
    };

    class TriMeshObject : public Object {
        std::shared_ptr<TriMesh> m_mesh;
    public:
        TriMeshObject(
            std::shared_ptr<TriMesh> mesh,
            const glm::mat4& transform,
            const std::shared_ptr<Material>& material
        ) : Object(mesh->obb(), transform, material), m_mesh(std::move(mesh)) {}

        virtual boost::optional<Intersection> find_intersection(const Ray& r) const;
    };
}

#endif
