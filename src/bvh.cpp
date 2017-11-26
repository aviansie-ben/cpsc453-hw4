#include <algorithm>

#include "bvh.hpp"

namespace hw4 {
    bool BoundingBox::intersects(const Ray& r) const {
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
