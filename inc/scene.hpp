#ifndef HW4_SCENE_HPP
#define HW4_SCENE_HPP

#include <memory>
#include <vector>

#include "object.hpp"

namespace hw4 {
    class Scene {
        std::vector<std::unique_ptr<Object>> m_objects;
    public:
        Scene() {}

        const std::vector<std::unique_ptr<Object>>& objects() const { return this->m_objects; }
        std::vector<std::unique_ptr<Object>>& objects() { return this->m_objects; }
    };
}

#endif
