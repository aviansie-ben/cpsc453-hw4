#ifndef HW4_BVH_HPP
#define HW4_BVH_HPP

#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "ray.hpp"

namespace hw4 {
    class BoundingBox {
        glm::vec3 m_min;
        glm::vec3 m_max;
    public:
        BoundingBox() : m_min(glm::vec3(0)), m_max(glm::vec3(0)) {}
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
            float tmin, tmax;

            {
                float tx1 = (this->m_min.x - r.origin().x) * r.inv_direction().x;
                float tx2 = (this->m_max.x - r.origin().x) * r.inv_direction().x;

                tmin = std::min(tx1, tx2);
                tmax = std::max(tx1, tx2);
            }

            {
                float ty1 = (this->m_min.y - r.origin().y) * r.inv_direction().y;
                float ty2 = (this->m_max.y - r.origin().y) * r.inv_direction().y;

                tmin = std::max(tmin, std::min(ty1, ty2));
                tmax = std::min(tmax, std::max(ty1, ty2));
            }

            {
                float tz1 = (this->m_min.z - r.origin().z) * r.inv_direction().z;
                float tz2 = (this->m_max.z - r.origin().z) * r.inv_direction().z;

                tmin = std::max(tmin, std::min(tz1, tz2));
                tmax = std::min(tmax, std::max(tz1, tz2));
            }

            return tmax >= tmin && tmax > 0;
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

    // Adapted from http://www.forceflow.be/2013/10/07/morton-encodingdecoding-through-bit-interleaving-implementations/
    inline uint64_t morton_code(uint32_t x, uint32_t y, uint32_t z) {
        auto split_by_3 = [](uint32_t a) {
            // IMPORTANT: The source above uses the bottom 21 bits of the number. However, since our
            // numbers are normalized to have a maximum of 0xffffffff, we need the *top* 21 bits in
            // order to get a usable result.
            uint64_t val = a >> 11;

            val = (val | val << 32) & 0x1f00000000ffff;
            val = (val | val << 16) & 0x1f0000ff0000ff;
            val = (val | val << 8) & 0x100f00f00f00f00f;
            val = (val | val << 4) & 0x10c30c30c30c30c3;
            val = (val | val << 2) & 0x1249249249249249;

            return val;
        };

        return split_by_3(x) | (split_by_3(y) << 1) | (split_by_3(z) << 2);
    }

    inline uint64_t morton_code(const glm::vec3& pos) {
        assert(pos.x >= 0 && pos.x <= 1);
        assert(pos.y >= 0 && pos.y <= 1);
        assert(pos.z >= 0 && pos.z <= 1);

        return morton_code(
            static_cast<uint32_t>(pos.x * std::numeric_limits<uint32_t>::max()),
            static_cast<uint32_t>(pos.y * std::numeric_limits<uint32_t>::max()),
            static_cast<uint32_t>(pos.z * std::numeric_limits<uint32_t>::max())
        );
    }

    inline uint64_t morton_code(const BoundingBox& b, const BoundingBox& total) {
        auto v = (b.center() - total.min()) / (total.max() - total.min());

        // Deal with possible divisions by 0
        if (total.min().x == total.max().x) {
            v.x = 0;
        }

        if (total.min().y == total.max().y) {
            v.y = 0;
        }

        if (total.min().z == total.max().z) {
            v.z = 0;
        }

        return morton_code(v);
    }

    template <class T>
    class BVH {
    public:
        struct Node {
            BoundingBox box;

            std::unique_ptr<Node> left;
            std::unique_ptr<Node> right;

            T* object = nullptr;

            template <typename TFn>
            void search(const Ray& r, const TFn& fn) const {
                if (this->box.intersects(r)) {
                    if (this->object) {
                        fn(*this->object);
                    }

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

        // Construction is done using approximate agglomerative clustering based off of
        // http://graphics.cs.cmu.edu/projects/aac/aac_build.pdf
        template <typename TAABBFn>
        static BVH<T> construct(
            const std::vector<T*>& objects,
            size_t delta,
            const TAABBFn& aabb
        ) {
            if (objects.empty()) return BVH<T>();

            std::vector<std::tuple<T*, BoundingBox, uint64_t>> objects_sorted;
            auto box = BoundingBox(
                glm::vec3(std::numeric_limits<float>::infinity()),
                glm::vec3(-std::numeric_limits<float>::infinity())
            );

            for (T* o : objects) {
                auto o_box = aabb(*o);

                objects_sorted.emplace_back(o, o_box, 0);
                box = BoundingBox::combine(box, o_box);
            }

            for (auto& o : objects_sorted) {
                std::get<2>(o) = morton_code(std::get<1>(o), box);
            }

            std::sort(objects_sorted.begin(), objects_sorted.end(), [](const auto& t1, const auto& t2) {
                return std::get<2>(t1) < std::get<2>(t2);
            });

            return BVH<T>(construct_build_tree(
                objects_sorted,
                0,
                objects_sorted.size(),
                delta,
                62 // The top bit of the morton code we produce is always 0, so just ignore it
            ));
        }
    private:
        static std::unique_ptr<Node> construct_build_tree(
            const std::vector<std::tuple<T*, BoundingBox, uint64_t>>& objects,
            size_t start,
            size_t end,
            size_t delta,
            int bit
        ) {
            if (start == end) return nullptr;

            if ((end - start) <= delta || bit < 0) {
                return construct_combine_primitives(objects, start, end);
            } else {
                size_t part = construct_make_partition(objects, start, end, bit);

                if (part == start || part == end) {
                    return construct_build_tree(objects, start, end, delta, bit - 1);
                } else {
                    auto n = std::make_unique<Node>();

                    n->left = construct_build_tree(objects, start, part, delta, bit - 1);
                    n->right = construct_build_tree(objects, part, end, delta, bit - 1);

                    n->box = BoundingBox::combine(n->left->box, n->right->box);

                    return std::move(n);
                }
            }
        }

        static size_t construct_make_partition(
            const std::vector<std::tuple<T*, BoundingBox, uint64_t>>& objects,
            size_t start,
            size_t end,
            int bit
        ) {
            uint64_t bitmask = 1 << bit;

            while (start < end) {
                size_t mid = start + (end - start) / 2;

                if ((std::get<2>(objects[start]) & bitmask) == 0) {
                    start = mid + 1;
                } else {
                    end = mid;
                }
            }

            return start;
        }

        static std::unique_ptr<Node> construct_combine_primitives(
            const std::vector<std::tuple<T*, BoundingBox, uint64_t>>& objects,
            size_t start,
            size_t end
        ) {
            std::vector<std::unique_ptr<Node>> clusters;

            for (size_t i = start; i < end; i++) {
                const auto& o = objects[i];
                auto new_cluster = std::make_unique<Node>();

                new_cluster->box = std::get<1>(o);
                new_cluster->object = std::get<0>(o);

                clusters.push_back(std::move(new_cluster));
            }

            while (clusters.size() > 1) {
                float best = std::numeric_limits<float>::infinity();
                size_t i_best = 0;
                size_t j_best = 0;

                for (size_t i = 0; i < clusters.size(); i++) {
                    for (size_t j = i + 1; j < clusters.size(); j++) {
                        auto& ci = clusters[i];
                        auto& cj = clusters[j];
                        float dist = glm::distance(ci->box.center(), cj->box.center());

                        if (dist < best) {
                            best = dist;
                            i_best = i;
                            j_best = j;
                        }
                    }
                }

                auto new_cluster = std::make_unique<Node>();

                new_cluster->box = BoundingBox::combine(clusters[i_best]->box, clusters[j_best]->box);
                new_cluster->left = std::move(clusters[i_best]);
                new_cluster->right = std::move(clusters[j_best]);

                clusters.push_back(std::move(new_cluster));

                clusters.erase(clusters.begin() + j_best);
                clusters.erase(clusters.begin() + i_best);
            }

            return std::move(clusters[0]);
        }
    };
}

#endif
