#ifndef HW4_MATERIAL_HPP
#define HW4_MATERIAL_HPP

namespace hw4 {
    struct PointMaterial {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float shininess;

        float reflectance;
    };

    class Material {
        glm::vec3 m_ambient;
        glm::vec3 m_diffuse;
        glm::vec3 m_specular;
        float m_shininess = 0;

        float m_reflectance = 0;
    public:
        PointMaterial at_point(glm::vec2 texcoord) const {
            return PointMaterial {
                .ambient = this->m_ambient,
                .diffuse = this->m_diffuse,
                .specular = this->m_specular,
                .shininess = this->m_shininess,

                .reflectance = this->m_reflectance
            };
        }

        static Material reflective(
            const Material& diffuse_part,
            float reflectance
        ) {
            Material m = diffuse_part;

            m.m_reflectance = reflectance;

            return m;
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

            return m;
        }
    };
}

#endif
