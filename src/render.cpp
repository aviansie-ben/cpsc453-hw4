#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "render.hpp"

namespace hw4 {
    void Image::save_as_ppm(const boost::filesystem::path& path) const {
        boost::filesystem::ofstream f(path);

        if (!f) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Failed to open " << path.native() << " for writing!";

                return ss.str();
            })());
        }

        f << "P3\n" << this->m_size.x << " " << this->m_size.y << " 255\n";

        for (int y = 0; y < this->m_size.y; y++) {
            for (int x = 0; x < this->m_size.x; x++) {
                const auto& p = this->m_data[x + (y * this->m_size.x)];

                f << static_cast<int>(p.r) << " "
                  << static_cast<int>(p.g) << " "
                  << static_cast<int>(p.b) << "\n";
            }
        }
    }

    Image RayTraceRenderer::render(const Scene& scene, const glm::mat4& view_matrix) const {
        float img_plane_distance = this->m_size.x / (std::tan(this->m_hfov / 2) * 2.0f);

        return this->render_patch(
            scene,
            glm::inverse(view_matrix),
            img_plane_distance,
            glm::ivec2(0, 0),
            this->m_size
        );
    }

    Image RayTraceRenderer::render_patch(
        const Scene& scene,
        const glm::mat4& inv_view_matrix,
        float img_plane_distance,
        glm::ivec2 start,
        glm::ivec2 size
    ) const {
        Image img(size);

        for (int y = 0; y < size.y; y++) {
            for (int x = 0; x < size.x; x++) {
                auto ipos = glm::ivec2(start.x + x, start.y + y);
                auto color = this->render_ray(
                    scene,
                    inv_view_matrix * Ray::between(
                        glm::vec3(0),
                        glm::vec3(
                            ipos.x + 0.5 - this->m_size.x / 2,
                            -ipos.y - 0.5 + this->m_size.y / 2,
                            img_plane_distance
                        )
                    )
                );

                // Perform gamma correction
                color = glm::vec3(
                    std::pow(color.r, 1.0/2.2),
                    std::pow(color.g, 1.0/2.2),
                    std::pow(color.b, 1.0/2.2)
                );

                img.set_pixel(ipos, color);
            }
        }

        return std::move(img);
    }

    glm::vec3 RayTraceRenderer::render_ray(
        const Scene& scene,
        const Ray& ray,
        int recursion
    ) const {
        if (recursion > this->m_max_recursion) return glm::vec3(0);

        float depth = std::numeric_limits<float>::infinity();
        Intersection i;

        scene.bvh().search(ray, [&](auto& n) {
            for (Object* o : n.objects) {
                if (o->aabb().intersects(ray)) {
                    float dist_mult;
                    Ray obj_ray = ray.transform(o->inv_transform(), dist_mult);
                    boost::optional<Intersection> intersection = o->find_intersection(obj_ray);

                    if (!intersection) continue;

                    float new_depth = intersection->distance() * dist_mult;

                    if (new_depth < depth) {
                        depth = intersection->distance() * dist_mult;
                        i = intersection->transform(o->transform(), dist_mult);
                    }
                }
            }
        });

        if (depth != std::numeric_limits<float>::infinity()) {
            auto mat = i.material();
            auto result = glm::vec3();

            for (const auto& plight : scene.point_lights()) {
                result += this->render_point_light(scene, ray, i, mat, *plight);
            }

            if (mat.reflectance > 0) {
                result += mat.reflectance * this->render_ray(
                    scene,
                    Ray(i.point(), glm::reflect(ray.direction(), i.normal())).adjust(0.001f),
                    recursion + 1
                );
            }

            return result;
        } else {
            return glm::vec3(0);
        }
    }

    glm::vec3 RayTraceRenderer::render_point_light(
        const Scene& scene,
        const Ray& ray,
        const Intersection& i,
        const PointMaterial& mat,
        const PointLight& plight
    ) const {
        glm::vec3 result;

        float attenuation = plight.attenuation(glm::distance(plight.pos(), i.point()));

        result += plight.ambient() * mat.ambient;

        if (this->is_visible(scene, i.point(), plight.pos())) {
            glm::vec3 obj_to_light = glm::normalize(plight.pos() - i.point());

            result += std::max(glm::dot(i.normal(), obj_to_light), 0.0f)
                * plight.diffuse()
                * mat.diffuse;

            result += std::pow(std::max(glm::dot(-ray.direction(), glm::reflect(-obj_to_light, i.normal())), 0.0f), mat.shininess)
                * plight.specular()
                * mat.specular;
        }

        return result * attenuation;
    }

    bool RayTraceRenderer::is_visible(const Scene& scene, glm::vec3 from, glm::vec3 to) const {
        Ray ray = Ray::between(from, to).adjust(0.001f);
        float dist = std::max(glm::distance(from, to) - 0.001f, 0.0f);
        bool occluded = false;

        // TODO Optimize this
        scene.bvh().search(ray, [&](auto& n) {
            for (Object* o : n.objects) {
                if (o->aabb().intersects(ray)) {
                    float dist_mult;
                    Ray obj_ray = ray.transform(o->inv_transform(), dist_mult);
                    boost::optional<Intersection> intersection = o->find_intersection(obj_ray);

                    if (!intersection) continue;

                    if (intersection->distance() * dist_mult <= dist) {
                        occluded = true;
                    }
                }
            }
        });

        return !occluded;
    }
}
