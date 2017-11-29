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

        bool intersects(const Ray& r) const {
            // Calculate the range for t in the x direction. Note that if r has no x component, the
            // range will properly come out to be -Infinity to Infinity.
            float tmin = (this->m_min.x - r.origin().x) * r.inv_direction().x;
            float tmax = (this->m_max.x - r.origin().x) * r.inv_direction().x;

            if (tmin > tmax) {
                std::swap(tmin, tmax);
            }

            {
                // Calculate the range for t in the y direction.
                float tymin = (this->m_min.y - r.origin().y) * r.inv_direction().y;
                float tymax = (this->m_max.y - r.origin().y) * r.inv_direction().y;

                if (tymin > tymax) {
                    std::swap(tymin, tymax);
                }

                // If the calculated range was outside of the existing range, then the ray does not
                // intersect.
                if (tymin > tmax || tymax < tmin) {
                    return false;
                }

                // Update the known range for t.
                tmin = std::max(tmin, tymin);
                tmax = std::min(tmax, tymax);
            }

            {
                // Calculate the range for t in the z direction.
                float tzmin = (this->m_min.z - r.origin().z) * r.inv_direction().z;
                float tzmax = (this->m_max.z - r.origin().z) * r.inv_direction().z;

                if (tzmin > tzmax) {
                    std::swap(tzmin, tzmax);
                }

                // If the calculated range was outside of the existing range, then the ray does not
                // intersect.
                if (tzmin > tmax || tzmax < tmin) {
                    return false;
                }

                // We could update tmin and tmax here, but we don't do any more checks, so that would be
                // redundant.
            }

            // Now that we know the range for t, we can assume that the ray will intersect if the range
            // for t includes some non-negative number.
            return tmax > 0;
        }

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

            template <typename TFn>
            void search(const Ray& r, const TFn& fn) const {
                if (this->box.intersects(r)) {
                    fn(*this);

                    if (this->left) this->left->search(r, fn);
                    if (this->right) this->right->search(r, fn);
                }
            }
        };
    private:
        std::unique_ptr<Node> m_root;
    public:
        BVH() : m_root(nullptr) {}
        BVH(std::unique_ptr<Node> root) : m_root(std::move(root)) {}

        const std::unique_ptr<Node>& root() const { return this->m_root; }

        template <typename TFn>
        void search(const Ray& r, const TFn& fn) const {
            if (this->m_root) {
                this->m_root->search(r, fn);
            }
        }

        template <typename TAABBFn>
        static BVH<T> construct(
            const std::vector<T*>& objects,
            TAABBFn aabb
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
