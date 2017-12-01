#ifndef HW4_SCENE_HPP
#define HW4_SCENE_HPP

#include <memory>
#include <vector>

#include "bvh.hpp"
#include "light.hpp"
#include "object.hpp"

namespace hw4 {
    class Scene {
        std::vector<std::unique_ptr<Object>> m_objects;
        std::vector<std::unique_ptr<PointLight>> m_point_lights;

        BVH<Object> m_bvh;
    public:
        Scene() {}

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
    };
}

#endif
