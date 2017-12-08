#ifndef HW4_TEXTURE_HPP
#define HW4_TEXTURE_HPP

#include <cassert>
#include <memory>

#include <glm/glm.hpp>

namespace hw4 {
    inline glm::vec3 interpolate(
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c,
        const glm::vec3& d,
        float x,
        float y
    ) {
        assert(x >= 0 && x <= 1);
        assert(y >= 0 && y <= 1);

        return glm::mix(glm::mix(a, b, x), glm::mix(c, d, x), y);

        glm::vec3 ab = (x - 1) * a + x * b;
        glm::vec3 cd = (x - 1) * c + x * d;

        return (y - 1) * ab + y * cd;
    }

    class Texture2D {
        glm::ivec2 m_size;
        std::unique_ptr<glm::vec3[]> m_ptr;

        glm::vec3 get_pixel(int x, int y) const {
            assert(this->m_ptr);
            assert(x >= 0 && x < this->m_size.x);
            assert(y >= 0 && y < this->m_size.y);

            return this->m_ptr[y * this->m_size.x + x];
        }
    public:
        Texture2D() {}
        Texture2D(glm::ivec2 size, std::unique_ptr<glm::vec3[]> ptr)
            : m_size(size), m_ptr(std::move(ptr)) {}

        glm::vec3 get(glm::vec2 tc) const {
            if (tc.x < 0 || tc.x > 1 || tc.y < 0 || tc.y > 1) return glm::vec3();

            tc *= glm::vec2(this->m_size.x - 1, this->m_size.y - 1);

            return interpolate(
                this->get_pixel(std::floor(tc.x), std::floor(tc.y)),
                this->get_pixel(std::ceil(tc.x), std::floor(tc.y)),
                this->get_pixel(std::floor(tc.x), std::ceil(tc.y)),
                this->get_pixel(std::ceil(tc.x), std::ceil(tc.y)),
                tc.x - std::floor(tc.x),
                tc.y - std::floor(tc.y)
            );
        }

        static std::shared_ptr<Texture2D> load(const std::string& path);
    };
}

#endif
