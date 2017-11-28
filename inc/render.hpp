#ifndef HW4_RENDER_HPP
#define Hw4_RENDER_HPP

#include <algorithm>
#include <functional>
#include <memory>

#include <boost/filesystem.hpp>
#include <glm/glm.hpp>

#include "scene.hpp"

namespace hw4 {
    class Image {
        struct pixel {
            unsigned char r;
            unsigned char g;
            unsigned char b;
        };

        glm::ivec2 m_size;
        std::unique_ptr<pixel[]> m_data;
    public:
        Image() : m_size(glm::ivec2(0)), m_data(nullptr) {}
        Image(glm::ivec2 size)
            : m_size(size), m_data(std::make_unique<pixel[]>(size.x * size.y)) {}

        glm::ivec2 size() const { return this->m_size; }

        Image& fill(glm::vec3 value) {
            std::fill(
                &this->m_data[0],
                &this->m_data[this->m_size.x * this->m_size.y],
                pixel {
                    static_cast<unsigned char>(std::round(glm::clamp(value.r, 0.0f, 1.0f) * 255.0f)),
                    static_cast<unsigned char>(std::round(glm::clamp(value.g, 0.0f, 1.0f) * 255.0f)),
                    static_cast<unsigned char>(std::round(glm::clamp(value.b, 0.0f, 1.0f) * 255.0f))
                }
            );

            return *this;
        }

        Image& set_pixel(glm::ivec2 pos, glm::vec3 value) {
            assert(pos.x >= 0 && pos.x < this->m_size.x);
            assert(pos.y >= 0 && pos.y < this->m_size.y);

            this->m_data[static_cast<size_t>(pos.x + pos.y * this->m_size.x)] = pixel {
                static_cast<unsigned char>(std::round(glm::clamp(value.r, 0.0f, 1.0f) * 255.0f)),
                static_cast<unsigned char>(std::round(glm::clamp(value.g, 0.0f, 1.0f) * 255.0f)),
                static_cast<unsigned char>(std::round(glm::clamp(value.b, 0.0f, 1.0f) * 255.0f))
            };

            return *this;
        }

        glm::vec3 get_pixel(glm::ivec2 pos) const {
            assert(pos.x >= 0 && pos.x < this->m_size.x);
            assert(pos.y >= 0 && pos.y < this->m_size.y);

            pixel p = this->m_data[static_cast<size_t>(pos.x + pos.y * this->m_size.x)];

            return glm::vec3(p.r / 255.0f, p.g / 255.0f, p.b / 255.0f);
        }

        Image& copy_data(const Image& img, glm::ivec2 pos) {
            assert(pos.x >= 0 && pos.x + img.m_size.x <= this->m_size.x);
            assert(pos.y >= 0 && pos.y + img.m_size.y <= this->m_size.y);

            // Since image rows are stored separately, each row needs to be copied individually.
            for (int y = 0; y < img.m_size.y; y++) {
                std::copy(
                    &img.m_data[y * img.m_size.x],
                    &img.m_data[(y + 1) * img.m_size.x],
                    &this->m_data[pos.x + (y + pos.y) * this->m_size.x]
                );
            }

            return *this;
        }

        void save_as_ppm(const boost::filesystem::path& path) const;
    };

    class RayTraceRenderer {
        glm::ivec2 m_size;
        float m_hfov;
        int m_max_recursion;
        int m_supersample_level;

        float m_img_plane_distance;
        float m_sample_spacing;
        float m_sample_mult;

        void update_params();
    public:
        RayTraceRenderer(glm::ivec2 size, float hfov, int max_recursion, int supersample_level)
            : m_size(size), m_hfov(hfov), m_max_recursion(max_recursion),
              m_supersample_level(supersample_level) {
            this->update_params();
        }

        glm::ivec2 size() const { return this->m_size; }
        float hfov() const { return this->m_hfov; }
        int max_recursion() const { return this->m_max_recursion; }
        int supersample_level() const { return this->m_supersample_level; }

        Image render(
            const Scene& scene,
            const glm::mat4& view_matrix,
            std::function<void (float)> progress_callback
        ) const;
        Image render_patch(
            const Scene& scene,
            const glm::mat4& inv_view_matrix,
            glm::ivec2 start,
            glm::ivec2 size
        ) const;
        glm::vec3 render_pixel(
            const Scene& scene,
            const glm::mat4& inv_view_matrix,
            glm::ivec2 pos
        ) const;
        glm::vec3 render_ray(
            const Scene& scene,
            const Ray& ray
        ) const {
            return this->render_ray(scene, ray, 0);
        }
    private:
        glm::vec3 render_ray(
            const Scene& scene,
            const Ray& ray,
            int recursion
        ) const;
        glm::vec3 render_point_light(
            const Scene& scene,
            const Ray& ray,
            const Intersection& intersection,
            const PointMaterial& material,
            const PointLight& point_light
        ) const;
        bool is_visible(const Scene& scene, glm::vec3 from, glm::vec3 to) const;
    };
}

#endif
