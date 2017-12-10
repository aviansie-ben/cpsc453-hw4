#ifndef HW4_MATERIAL_HPP
#define HW4_MATERIAL_HPP

#include "texture.hpp"

namespace hw4 {
    struct PointMaterial {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float shininess;

        float opacity;

        float reflectance;
        float transmittance;
        float refractive_index;
    };

    class Material {
        glm::vec3 m_ambient;
        glm::vec3 m_diffuse;
        glm::vec3 m_specular;
        float m_shininess = 0;

        std::shared_ptr<Texture2D> m_diffuse_texture;
        std::shared_ptr<Texture2D> m_ao_texture;

        float m_opacity = 1;

        float m_reflectance = 0;
        float m_transmittance = 0;
        float m_refractive_index = 1;
    public:
        PointMaterial at_point(glm::vec2 texcoord) const {
            auto m = PointMaterial {
                .ambient = this->m_ambient,
                .diffuse = this->m_diffuse,
                .specular = this->m_specular,
                .shininess = this->m_shininess,

                .opacity = this->m_opacity,

                .reflectance = this->m_reflectance,
                .transmittance = this->m_transmittance,
                .refractive_index = this->m_refractive_index
            };

            if (this->m_diffuse_texture) {
                auto d = this->m_diffuse_texture->get(texcoord);

                m.ambient *= d;
                m.diffuse *= d;
            }

            if (this->m_ao_texture) {
                m.ambient *= this->m_ao_texture->get(texcoord);
            }

            return m;
        }

        static Material textured(
            const Material& base,
            std::shared_ptr<Texture2D> diffuse_texture,
            std::shared_ptr<Texture2D> ao_texture
        ) {
            Material m = base;

            m.m_diffuse_texture = std::move(diffuse_texture);
            m.m_ao_texture = std::move(ao_texture);

            return m;
        }

        static Material translucent(
            const Material& diffuse_part,
            float opacity,
            float transmittance,
            float refractive_index
        ) {
            Material m = diffuse_part;

            m.m_opacity = opacity;
            m.m_transmittance = transmittance;
            m.m_refractive_index = refractive_index;

            return m;
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
