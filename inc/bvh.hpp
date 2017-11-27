#ifndef HW4_BVH_HPP
#define HW4_BVH_HPP

#include <deque>
#include <functional>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "ray.hpp"

namespace hw4 {
    class BoundingBox {
        glm::vec3 m_min;
        glm::vec3 m_max;
    public:
        BoundingBox(glm::vec3 min, glm::vec3 max) : m_min(min), m_max(max) {}

        const glm::vec3& min() const { return this->m_min; }
        const glm::vec3& max() const { return this->m_max; }

        glm::vec3 size() const { return this->m_max - this->m_min; }
        glm::vec3 center() const { return 0.5f * (this->m_max + this->m_min); }

        std::array<glm::vec3, 8> corners() const {
            return std::array<glm::vec3, 8> {
                this->m_min,
                glm::vec3(this->m_max.x, this->m_min.y, this->m_min.z),
                glm::vec3(this->m_min.x, this->m_max.y, this->m_min.z),
                glm::vec3(this->m_max.x, this->m_max.y, this->m_min.z),
                glm::vec3(this->m_min.x, this->m_min.y, this->m_max.z),
                glm::vec3(this->m_max.x, this->m_min.y, this->m_max.z),
                glm::vec3(this->m_min.x, this->m_max.y, this->m_max.z),
                this->m_max
            };
        }

        bool intersects(const Ray& r) const;

        static BoundingBox combine(BoundingBox a, BoundingBox b) {
            return BoundingBox(
                glm::vec3(
                    std::min(a.min().x, b.min().x),
                    std::min(a.min().y, b.min().y),
                    std::min(a.min().z, b.min().z)
                ),
                glm::vec3(
                    std::max(a.max().x, b.max().x),
                    std::max(a.max().y, b.max().y),
                    std::max(a.max().z, b.max().z)
                )
            );
        }
    };

    BoundingBox operator *(const glm::mat4& m, const BoundingBox& b);
    inline BoundingBox operator *(const BoundingBox& b, const glm::mat4& m) { return m * b; }

    template <class T>
    class BVH {
    public:
        struct Node {
            BoundingBox box;

            std::unique_ptr<Node> left;
            std::unique_ptr<Node> right;

            std::vector<T*> objects;
        };
    private:
        std::unique_ptr<Node> m_root;
    public:
        BVH() : m_root(nullptr) {}
        BVH(std::unique_ptr<Node> root) : m_root(std::move(root)) {}

        const std::unique_ptr<Node>& root() const { return this->m_root; }

        void search(const Ray& r, std::function<void (Node&)> fn) const {
            if (!this->m_root) return;

            std::deque<Node*> nodes { this->m_root.get() };

            while (!nodes.empty()) {
                auto node = nodes.front();

                if (node->box.intersects(r)) {
                    if (node->left) nodes.push_back(node->left.get());
                    if (node->right) nodes.push_back(node->right.get());

                    fn(*node);
                }

                nodes.pop_front();
            }
        }

        static BVH<T> construct(
            const std::vector<T*>& objects,
            std::function<BoundingBox (const T&)> aabb
        ) {
            if (objects.empty()) return BVH<T>();

            // TODO Actually build a BVH
            auto n = std::make_unique<Node>(Node {
                .box = aabb(*objects[0])
            });

            for (T* o : objects) {
                n->box = BoundingBox::combine(n->box, aabb(*o));
                n->objects.push_back(o);
            }

            return BVH<T>(std::move(n));
        }
    };
}

#endif
