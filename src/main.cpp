#include <atomic>
#include <chrono>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>

#include "render.hpp"
#include "scene.hpp"

namespace hw4 {
    void print_image(const Image& img, std::ostream& s) {
        for (int y = 0; y < img.size().y; y += 2) {
            for (int x = 0; x < img.size().x; x++) {
                auto pt = img.get_pixel(glm::ivec2(x, y));
                auto pb = y < img.size().y - 1 ? img.get_pixel(glm::ivec2(x, y + 1)) : glm::vec3(0);

                s << "\033[38;2;"
                  << static_cast<int>(pt.r * 255) << ";"
                  << static_cast<int>(pt.g * 255) << ";"
                  << static_cast<int>(pt.b * 255) << "m"
                  << "\033[48;2;"
                  << static_cast<int>(pb.r * 255) << ";"
                  << static_cast<int>(pb.g * 255) << ";"
                  << static_cast<int>(pb.b * 255) << "m";

                s << "▀";
            }

            s << "\033[0m\n";
        }

        s << std::flush;
    }

    void wait_with_spinner(const std::future<void>& f) {
        int state = 0;

        std::cout << " ";

        do {
            switch (state) {
            case 0:
                std::cout << "-";
                break;
            case 1:
                std::cout << "\\";
                break;
            case 2:
                std::cout << "|";
                break;
            case 3:
                std::cout << "/";
                break;
            }

            std::cout << "\033[1D" << std::flush;

            state++;
            if (state == 4) state = 0;
        } while (f.wait_for(std::chrono::milliseconds(250)) == std::future_status::timeout);

        std::cout << "DONE" << std::endl;
    }

    void wait_with_spinner_and_progress(
        const std::future<void>& f,
        const std::function<float ()>& progress_fn
    ) {
        int state = 0;

        std::cout << " ";

        do {
            std::ostringstream ss;

            switch (state) {
            case 0:
                ss << "-";
                break;
            case 1:
                ss << "\\";
                break;
            case 2:
                ss << "|";
                break;
            case 3:
                ss << "/";
                break;
            }

            ss << " ["
               << std::fixed << std::setprecision(2) << std::setw(6) << progress_fn() * 100
               << "%]";

            std::cout << "\033[K" << ss.str();
            std::cout << "\033[" << ss.str().size() << "D" << std::flush;

            state++;
            if (state == 4) state = 0;
        } while (f.wait_for(std::chrono::milliseconds(250)) == std::future_status::timeout);

        std::cout << "\033[KDONE" << std::endl;
    }

    void render_with_progress(
        const RayTraceRenderer& r,
        const Scene& s,
        const glm::mat4& vm,
        Image& i
    ) {
        std::atomic<float> progress(0);

        wait_with_spinner_and_progress(
            std::async([&]() {
                i = r.render(s, vm, [&](float p) {
                    progress.store(p);
                });
            }),
            [&]() {
                return progress.load();
            }
        );
    }

    void show_scene(const Scene& scene) {
        RayTraceRenderer render(glm::ivec2(160, 120), glm::radians(90.0f), 5, 2);
        Image img;

        std::cout << "Generating preview...";
        render_with_progress(
            render,
            scene,
            glm::mat4(),
            img
        );

        print_image(img, std::cout);
    }

    void render_scene(const Scene& scene) {
        RayTraceRenderer render(glm::ivec2(640, 480), glm::radians(90.0f), 5, 4);
        Image img;

        std::cout << "Generating full-size image...";
        render_with_progress(
            render,
            scene,
            glm::mat4(),
            img
        );

        img.save_as_ppm("test.ppm");
    }

    void populate_scene(Scene& scene) {
        auto mat = std::make_shared<Material>(Material::diffuse(
            glm::vec3(0.7, 0.3, 0.3),
            glm::vec3(0.7, 0.3, 0.3),
            glm::vec3(0.5),
            10
        ));
        auto mat2 = std::make_shared<Material>(Material::reflective(
            Material::diffuse(
                glm::vec3(0, 0, 0.1),
                glm::vec3(0, 0, 0.1),
                glm::vec3(1),
                500
            ),
            0.3
        ));
        auto mat3 = std::make_shared<Material>(Material::reflective(
            Material::diffuse(
                glm::vec3(0, 0.01, 0),
                glm::vec3(0, 0.01, 0),
                glm::vec3(1),
                500
            ),
            0.9
        ));
        auto mdl_knight = TriMesh::load_mesh("knight.obj");

        scene.objects().push_back(std::make_unique<TriMeshObject>(
                mdl_knight,
                glm::translate(glm::mat4(), glm::vec3(0, -1, -2)),
                mat
        ));
        scene.objects().push_back(
            std::make_unique<SphereObject>(0.5, glm::vec3(0, 0, 2), mat2)
        );
        scene.objects().push_back(std::make_unique<TriMeshObject>(
            std::make_shared<TriMesh>(
                std::vector<Vertex> {
                    Vertex { glm::vec3(2, 2, 0), glm::vec3(0, 0, -1), glm::vec2(0, 0) },
                    Vertex { glm::vec3(-2, 2, 0), glm::vec3(0, 0, -1), glm::vec2(0, 0) },
                    Vertex { glm::vec3(-2, -2, 0), glm::vec3(0, 0, -1), glm::vec2(0, 0) },
                    Vertex { glm::vec3(2, -2, 0), glm::vec3(0, 0, -1), glm::vec2(0, 0) }
                },
                std::vector<Triangle> {
                    Triangle { 0, 2, 1 },
                    Triangle { 0, 3, 2 }
                }
            ),
            glm::rotate(glm::translate(glm::mat4(), glm::vec3(-1, 0, 4)), glm::radians(-30.0f), glm::vec3(0, 1, 0)),
            mat
        ));
        scene.objects().push_back(std::make_unique<TriMeshObject>(
            std::make_shared<TriMesh>(
                std::vector<Vertex> {
                    Vertex { glm::vec3(2, 2, 0), glm::vec3(0, 0, -1), glm::vec2(0, 0) },
                    Vertex { glm::vec3(-2, 2, 0), glm::vec3(0, 0, -1), glm::vec2(0, 0) },
                    Vertex { glm::vec3(-2, -2, 0), glm::vec3(0, 0, -1), glm::vec2(0, 0) },
                    Vertex { glm::vec3(2, -2, 0), glm::vec3(0, 0, -1), glm::vec2(0, 0) }
                },
                std::vector<Triangle> {
                    Triangle { 0, 2, 1 },
                    Triangle { 0, 3, 2 }
                }
            ),
            glm::rotate(glm::translate(glm::mat4(), glm::vec3(1, 0, 4)), glm::radians(30.0f), glm::vec3(0, 1, 0)),
            mat3
        ));

        scene.point_lights().push_back(std::make_unique<PointLight>(
            glm::vec3(0, 1, 1),
            glm::vec3(0.005),
            glm::vec3(0.2),
            glm::vec3(0.5),
            glm::vec3(1, 0, 0.3)
        ));
    }

    extern "C" int main(int argc, char** argv) {
        Scene scene;

        std::cout << "Loading scene...";
        wait_with_spinner(std::async([&]() {
            populate_scene(scene);
        }));

        std::cout << "Building scene BVH...";
        wait_with_spinner(std::async([&]() {
            scene.regen_bvh();
        }));

        show_scene(scene);
        render_scene(scene);

        return 0;
    }
}
