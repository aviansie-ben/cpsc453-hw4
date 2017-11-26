#ifndef HW4_RAY_HPP
#define HW4_RAY_HPP

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
}

#endif
