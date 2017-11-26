#ifndef HW4_OBJECT_HPP
#define HW4_OBJECT_HPP

#include <array>

#include <boost/optional.hpp>
#include <glm/glm.hpp>

namespace hw4 {
    class Ray {
        glm::vec3 m_origin;
        glm::vec3 m_direction;
        glm::vec3 m_inv_direction;

        Ray(glm::vec3 origin, glm::vec3 direction, glm::vec3 inv_direction)
            : m_origin(origin), m_direction(direction), m_inv_direction(inv_direction) {}
    public:
        Ray(glm::vec3 origin, glm::vec3 direction)
            : m_origin(origin), m_direction(direction), m_inv_direction(1.0f / direction) {}

        const glm::vec3& origin() const { return this->m_origin; }
        const glm::vec3& direction() const { return this->m_direction; }
        const glm::vec3& inv_direction() const { return this->m_inv_direction; }

        Ray adjust(float distance) const {
            return Ray(
                this->m_origin + this->m_direction * distance,
                this->m_direction,
                this->m_inv_direction
            );
        }

        Ray transform(const glm::mat4& m, float& dist_mult) const {
            auto origin = glm::vec3(m * glm::vec4(this->m_origin, 1));
            auto direction = glm::mat3(m) * this->m_direction;

            dist_mult = 1 / glm::length(direction);

            return Ray(
                origin,
                direction * dist_mult
            );
        }

        static Ray between(glm::vec3 origin, glm::vec3 to) {
            return Ray(origin, glm::normalize(to - origin));
        }
    };

    inline Ray operator *(const glm::mat4& m, const Ray& r) {
        float dist_mult;

        return r.transform(m, dist_mult);
    }
    inline Ray operator *(const Ray& r, const glm::mat4& m) { return m * r; }

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

    class BoundingBox {
        glm::vec3 m_min;
        glm::vec3 m_max;
    public:
        BoundingBox(glm::vec3 min, glm::vec3 max) : m_min(min), m_max(max) {}

        const glm::vec3& min() const { return this->m_min; }
        const glm::vec3& max() const { return this->m_max; }

        glm::vec3 size() const { return this->m_max - this->m_min; }
        glm::vec3 center() const { return 0.5f * (this->m_max + this->m_min); }

        std::array<glm::vec3, 8> corners() const {
            return std::array<glm::vec3, 8> {
                this->m_min,
                glm::vec3(this->m_max.x, this->m_min.y, this->m_min.z),
                glm::vec3(this->m_min.x, this->m_max.y, this->m_min.z),
                glm::vec3(this->m_max.x, this->m_max.y, this->m_min.z),
                glm::vec3(this->m_min.x, this->m_min.y, this->m_max.z),
                glm::vec3(this->m_max.x, this->m_min.y, this->m_max.z),
                glm::vec3(this->m_min.x, this->m_max.y, this->m_max.z),
                this->m_max
            };
        }

        bool intersects(const Ray& r) const;
    };

    BoundingBox operator *(const glm::mat4& m, const BoundingBox& b);
    inline BoundingBox operator *(const BoundingBox& b, const glm::mat4& m) { return m * b; }

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
