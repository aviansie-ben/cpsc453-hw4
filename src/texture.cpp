#include <sstream>

#include <stb_image.h>

#include "texture.hpp"

namespace hw4 {
    std::shared_ptr<Texture2D> Texture2D::load(const std::string& path) {
        int width, height, channels;

        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 3);

        if (data == nullptr) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;
                ss << "Failed to read texture from file " << path;

                return ss.str();
            })());
        }

        auto data_copy = std::make_unique<glm::vec3[]>(width * height);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int off = y * width + x;

                data_copy[off] = glm::vec3(
                    static_cast<float>(data[off * 3]) / 255,
                    static_cast<float>(data[off * 3 + 1]) / 255,
                    static_cast<float>(data[off * 3 + 2]) / 255
                );
            }
        }

        stbi_image_free(data);

        return std::make_shared<Texture2D>(glm::ivec2(width, height), std::move(data_copy));
    }
}
