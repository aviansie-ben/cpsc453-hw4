#include "object.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace hw4 {
    SphereObject::SphereObject(float radius, glm::vec3 center)
        : Object(
            BoundingBox(glm::vec3(-1), glm::vec3(1)),
            glm::scale(glm::translate(glm::mat4(1.0), center), glm::vec3(radius))
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
            t
        );
    }
}
