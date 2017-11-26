#ifndef HW4_LIGHT_HPP
#define HW4_LIGHT_HPP

#include <glm/glm.hpp>

namespace hw4 {
    class PointLight {
        glm::vec3 m_pos;

        glm::vec3 m_ambient;
        glm::vec3 m_diffuse;
        glm::vec3 m_specular;

        glm::vec3 m_attenuation;
    public:
        PointLight(
            glm::vec3 pos,
            glm::vec3 ambient,
            glm::vec3 diffuse,
            glm::vec3 specular,
            glm::vec3 attenuation
        )
            : m_pos(pos), m_ambient(ambient), m_diffuse(diffuse), m_specular(specular),
              m_attenuation(attenuation) {}

        const glm::vec3& pos() const { return this->m_pos; }

        const glm::vec3& ambient() const { return this->m_ambient; }
        const glm::vec3& diffuse() const { return this->m_diffuse; }
        const glm::vec3& specular() const { return this->m_specular; }

        const glm::vec3& attenutation() const { return this->m_attenuation; }
        const float attenuation(float distance) const {
            return 1 / glm::dot(glm::vec3(1, distance, distance * distance), this->m_attenuation);
        }
    };
}

#endif
