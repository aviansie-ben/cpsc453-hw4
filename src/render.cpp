#include <cmath>
#include <iostream>
#include <limits>

#include "render.hpp"

namespace hw4 {
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

                img.set_pixel(
                    ipos,
                    this->render_ray(
                        scene,
                        inv_view_matrix * Ray::between(
                            glm::vec3(0),
                            glm::vec3(
                                ipos.x + 0.5 - this->m_size.x / 2,
                                -ipos.y - 0.5 + this->m_size.y / 2,
                                img_plane_distance
                            )
                        )
                    )
                );
            }
        }

        return std::move(img);
    }

    glm::vec3 RayTraceRenderer::render_ray(
        const Scene& scene,
        const Ray& ray
    ) const {
        float depth = std::numeric_limits<float>::infinity();

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
                    }
                }
            }
        });

        auto color = glm::vec3(1 - depth / 5.0);

        return color;
    }
}
