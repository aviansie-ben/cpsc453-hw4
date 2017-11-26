#ifndef HW4_MATERIAL_HPP
#define HW4_MATERIAL_HPP

namespace hw4 {
    struct PointMaterial {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float shininess;
    };

    class Material {
        glm::vec3 m_ambient;
        glm::vec3 m_diffuse;
        glm::vec3 m_specular;
        float m_shininess;
    public:
        PointMaterial at_point(glm::vec2 texcoord) const {
            return PointMaterial {
                .ambient = this->m_ambient,
                .diffuse = this->m_diffuse,
                .specular = this->m_specular,
                .shininess = this->m_shininess
            };
        }

        static Material diffuse(
            glm::vec3 ambient,
            glm::vec3 diffuse,
            glm::vec3 specular,
            float shininess
        ) {
            Material m;

            m.m_ambient = ambient;
            m.m_diffuse = diffuse;
            m.m_specular = specular;
            m.m_shininess = shininess;

            return std::move(m);
        }
    };
}

#endif
