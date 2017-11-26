#ifndef HW4_OBJECT_HPP
#define HW4_OBJECT_HPP

#include <array>

#include <boost/optional.hpp>
#include <glm/glm.hpp>

#include "bvh.hpp"
#include "ray.hpp"

namespace hw4 {
    class Intersection {
        glm::vec3 m_point;
        glm::vec3 m_normal;

        float m_distance;
    public:
        Intersection(glm::vec3 point, glm::vec3 normal, float distance)
            : m_point(point), m_normal(normal), m_distance(distance) {}

        const glm::vec3& point() const { return this->m_point; }
        const glm::vec3& normal() const { return this->m_normal; }
        float distance() const { return this->m_distance; }
    };

    class Object {
        glm::mat4 m_transform;
        glm::mat4 m_inv_transform;

        BoundingBox m_obb;
        BoundingBox m_aabb;
    public:
        Object(BoundingBox obb, const glm::mat4& transform)
            : m_transform(transform), m_inv_transform(glm::inverse(transform)), m_obb(obb),
              m_aabb(transform * obb) {}
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

        virtual boost::optional<Intersection> find_intersection(const Ray& r) const = 0;
    };

    class SphereObject : public Object {
    public:
        SphereObject(float radius, glm::vec3 center);

        virtual boost::optional<Intersection> find_intersection(const Ray& r) const;
    };
}

#endif
