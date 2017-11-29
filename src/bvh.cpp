#include <algorithm>

#include "bvh.hpp"

namespace hw4 {
    BoundingBox operator*(const glm::mat4& tf, const BoundingBox& aabb) {
        glm::vec3 min(std::numeric_limits<float>::infinity());
        glm::vec3 max(-std::numeric_limits<float>::infinity());

        for (auto& v : aabb.corners()) {
            v = glm::vec3(tf * glm::vec4(v, 1));

            if (v.x < min.x) min.x = v.x;
            if (v.x > max.x) max.x = v.x;

            if (v.y < min.y) min.y = v.y;
            if (v.y > max.y) max.y = v.y;

            if (v.z < min.z) min.z = v.z;
            if (v.z > max.z) max.z = v.z;
        }

        return BoundingBox(min, max);
    }
}
