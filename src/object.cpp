#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "object.hpp"

namespace hw4 {
    SphereObject::SphereObject(
        float radius,
        glm::vec3 center,
        const std::shared_ptr<Material>& material
    )
        : Object(
            BoundingBox(glm::vec3(-1), glm::vec3(1)),
            glm::scale(glm::translate(glm::mat4(1.0), center), glm::vec3(radius)),
            material
        ) {}

    boost::optional<Intersection> SphereObject::find_intersection(const Ray& r) const {
        float b = glm::dot(2.0f * r.direction(), r.origin());
        float c = glm::dot(r.origin(), r.origin()) - 1;

        float qterm = b * b - 4 * c;

        if (qterm < 0)
            return boost::none;

        float t = -(b + std::sqrt(qterm)) / 2;

        if (t <= 0)
            return boost::none;

        auto p = r.origin() + t * r.direction();

        return Intersection(
            p,
            p,
            glm::vec2(
                0.5 + std::atan2(p.z, p.x) / tau,
                0.5 + std::asin(p.y) * 2 / tau
            ),
            this->material(),
            t
        );
    }
}
