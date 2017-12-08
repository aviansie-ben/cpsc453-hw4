#ifndef HW4_SCENE_HPP
#define HW4_SCENE_HPP

#include <map>
#include <memory>
#include <vector>

#include <boost/filesystem.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "bvh.hpp"
#include "light.hpp"
#include "object.hpp"

namespace hw4 {
    class Camera {
        glm::vec3 m_pos;
        glm::vec3 m_look_at;
        glm::vec3 m_up;
        float m_hfov;
    public:
        Camera() : m_pos(0, 0, -1), m_look_at(0, 0, 0), m_up(0, 1, 0), m_hfov(glm::radians(90.0f)) {}
        Camera(glm::vec3 pos, glm::vec3 look_at, glm::vec3 up, float hfov)
            : m_pos(pos), m_look_at(look_at), m_up(up), m_hfov(hfov) {}

        const glm::vec3& pos() const { return this->m_pos; }
        const glm::vec3& look_at() const { return this->m_look_at; }
        const glm::vec3& up() const { return this->m_up; }
        float hfov() const { return this->m_hfov; }

        glm::mat4 as_matrix() const {
            return glm::lookAt(this->m_pos, this->m_look_at, this->m_up);
        }
    };

    class Scene {
        std::map<std::string, Camera> m_cameras;
        std::vector<std::unique_ptr<Object>> m_objects;
        std::vector<std::unique_ptr<PointLight>> m_point_lights;

        BVH<Object> m_bvh;
    public:
        Scene() {}

        const std::map<std::string, Camera>& cameras() const { return this->m_cameras; }
        std::map<std::string, Camera>& cameras() { return this->m_cameras; }

        const std::vector<std::unique_ptr<Object>>& objects() const { return this->m_objects; }
        std::vector<std::unique_ptr<Object>>& objects() { return this->m_objects; }

        const auto& point_lights() const { return this->m_point_lights; }
        auto& point_lights() { return this->m_point_lights; }

        const BVH<Object>& bvh() const { return this->m_bvh; }
        Scene& regen_mesh_bvhs(size_t delta) {
            std::vector<TriMesh*> meshes;

            for (const auto& obj : this->m_objects) {
                auto mesh_obj = dynamic_cast<TriMeshObject*>(obj.get());

                if (mesh_obj) {
                    if (std::find(meshes.begin(), meshes.end(), mesh_obj->mesh().get()) == meshes.end()) {
                        meshes.push_back(mesh_obj->mesh().get());
                    }
                }
            }

            for (auto mesh : meshes) {
                mesh->regen_bvh(delta);
            }

            return *this;
        }
        Scene& regen_bvh(size_t delta) {
            std::vector<Object*> objects;

            for (const auto& o : this->m_objects) {
                objects.push_back(o.get());
            }

            this->m_bvh = BVH<Object>::construct(
                objects,
                delta,
                [](const auto& o) { return o.aabb(); }
            );

            return *this;
        }

        static Scene load_scene(boost::filesystem::path path);
    };
}

#endif
