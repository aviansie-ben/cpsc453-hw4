#include <chrono>
#include <cmath>
#include <condition_variable>
#include <exception>
#include <iostream>
#include <limits>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <thread>

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

    void RayTraceRenderer::update_params() {
        this->m_img_plane_distance = this->m_size.x / (std::tan(this->m_hfov / 2) * 2.0f);
        this->m_sample_spacing = 1.0 / (this->m_supersample_level + 1);
        this->m_sample_mult = 1.0 / (this->m_supersample_level * this->m_supersample_level);
    }

    struct PatchJob {
        glm::ivec2 pos;
        glm::ivec2 size;
    };

    struct PatchJobResult {
        PatchJob job;
        Image img;
    };

    static void create_patch_jobs(
        std::queue<PatchJob>& job_queue,
        glm::ivec2 image_size,
        glm::ivec2 patch_size
    ) {
        for (int y = 0; y < image_size.y; y += patch_size.y) {
            for (int x = 0; x < image_size.x; x += patch_size.x) {
                glm::ivec2 size = glm::ivec2(
                    std::min(patch_size.x, image_size.x - x),
                    std::min(patch_size.y, image_size.y - y)
                );

                job_queue.push(PatchJob { .pos = glm::ivec2(x, y), .size = size });
            }
        }
    }

    Image RayTraceRenderer::render(
        const Scene& scene,
        const glm::mat4& view_matrix,
        std::function<void (float)> progress_callback
    ) const {
        auto inv_view_matrix = glm::inverse(view_matrix);
        auto next_progress_update = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);

        std::mutex mut;
        std::condition_variable cv;
        std::queue<PatchJob> job_queue;
        int total_jobs;
        int remaining_jobs;
        std::queue<PatchJobResult> result_queue;
        std::vector<std::thread> threads;

        create_patch_jobs(job_queue, this->m_size, glm::ivec2(8, 8));
        total_jobs = remaining_jobs = job_queue.size();

        for (int i = 0; i < 4; i++) {
            threads.push_back(std::thread([&]() {
                PatchJob job;

                {
                    std::unique_lock<std::mutex> lock(mut);

                    if (job_queue.empty()) return;
                    job = std::move(job_queue.front());
                    job_queue.pop();
                }

                while (true) {
                    Image img = this->render_patch(
                        scene,
                        inv_view_matrix,
                        job.pos,
                        job.size
                    );

                    {
                        std::unique_lock<std::mutex> lock(mut);

                        result_queue.push(PatchJobResult {
                            .job = job,
                            .img = std::move(img)
                        });
                        cv.notify_all();

                        if (job_queue.empty()) return;
                        job = std::move(job_queue.front());
                        job_queue.pop();
                    }
                }
            }));
        }

        Image img(this->m_size);

        while (true) {
            PatchJobResult r;

            {
                std::unique_lock<std::mutex> lock(mut);

                if (remaining_jobs <= 0) break;

                if (std::chrono::steady_clock::now() >= next_progress_update) {
                    progress_callback(static_cast<float>(total_jobs - remaining_jobs) / total_jobs);
                    next_progress_update = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
                }

                while (result_queue.empty()) {
                    cv.wait_for(lock, std::chrono::milliseconds(50));

                    if (std::chrono::steady_clock::now() >= next_progress_update) {
                        progress_callback(static_cast<float>(total_jobs - remaining_jobs) / total_jobs);
                        next_progress_update = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
                    }
                }

                r = std::move(result_queue.front());
                result_queue.pop();
                remaining_jobs--;
            }

            img.copy_data(r.img, r.job.pos);
        }

        for (auto& t : threads) {
            t.join();
        }

        return std::move(img);
    }

    Image RayTraceRenderer::render_patch(
        const Scene& scene,
        const glm::mat4& inv_view_matrix,
        glm::ivec2 start,
        glm::ivec2 size
    ) const {
        Image img(size);

        for (int y = 0; y < size.y; y++) {
            for (int x = 0; x < size.x; x++) {
                auto color = this->render_pixel(
                    scene,
                    inv_view_matrix,
                    start + glm::ivec2(x, y)
                );

                // Perform gamma correction
                color = glm::vec3(
                    std::pow(color.r, 1.0/2.2),
                    std::pow(color.g, 1.0/2.2),
                    std::pow(color.b, 1.0/2.2)
                );

                img.set_pixel(glm::ivec2(x, y), color);
            }
        }

        return std::move(img);
    }

    glm::vec3 RayTraceRenderer::render_pixel(
        const Scene& scene,
        const glm::mat4& inv_view_matrix,
        glm::ivec2 ipos
    ) const {
        glm::vec3 result;

        for (int y = 0; y < this->m_supersample_level; y++) {
            for (int x = 0; x < this->m_supersample_level; x++) {
                auto pos = glm::vec3(
                    ipos.x + this->m_sample_spacing * (x + 1) - this->m_size.x / 2,
                    -(ipos.y + this->m_sample_spacing * (y + 1) - this->m_size.y / 2),
                    this->m_img_plane_distance
                );

                result += this->render_ray(
                    scene,
                    inv_view_matrix * Ray::between(glm::vec3(0), pos)
                );
            }
        }

        return result * this->m_sample_mult;
    }

    glm::vec3 RayTraceRenderer::render_ray(
        const Scene& scene,
        const Ray& ray,
        int recursion
    ) const {
        if (recursion > this->m_max_recursion) return glm::vec3(0);

        float depth = std::numeric_limits<float>::infinity();
        Intersection i;

        scene.bvh().search(ray, [&](auto& o) {
            float dist_mult;
            Ray obj_ray = ray.transform(o.inv_transform(), dist_mult);
            boost::optional<Intersection> intersection = o.find_intersection(obj_ray);

            if (!intersection) return;

            float new_depth = intersection->distance() * dist_mult;

            if (new_depth < depth) {
                depth = intersection->distance() * dist_mult;
                i = intersection->transform(o.transform(), dist_mult);
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
        scene.bvh().search(ray, [&](auto& o) {
            float dist_mult;
            Ray obj_ray = ray.transform(o.inv_transform(), dist_mult);
            boost::optional<Intersection> intersection = o.find_intersection(obj_ray);

            if (!intersection) return;

            if (intersection->distance() * dist_mult <= dist) {
                occluded = true;
            }
        });

        return !occluded;
    }
}
