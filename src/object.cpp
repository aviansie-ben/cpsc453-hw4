#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>
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

        if (t <= 0) {
            t = -(b - std::sqrt(qterm)) / 2;

            if (t <= 0) return boost::none;
        }

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

    boost::optional<Intersection> TriMesh::find_intersection(const Ray& r, const Triangle& tri) const {
        const auto& a = this->m_vertices[tri.a];
        const auto& b = this->m_vertices[tri.b];
        const auto& c = this->m_vertices[tri.c];

        // Use the Möller-Trumbore algorithm for intersection
        auto ab = b.pos - a.pos;
        auto ac = c.pos - a.pos;

        auto pvec = glm::cross(r.direction(), ac);
        float det = glm::dot(ab, pvec);

        // If det is close to zero, then the ray is parallel to the triangle, so we can bail early.
        if (std::fabs(det) <= 1e-7f) return boost::none;

        float inv_det = 1.0 / det;

        auto tvec = r.origin() - a.pos;
        float u = glm::dot(tvec, pvec) * inv_det;

        if (u < 0 || u > 1) return boost::none;

        auto qvec = glm::cross(tvec, ab);
        float v = glm::dot(r.direction(), qvec) * inv_det;

        if (v < 0 || u + v > 1) return boost::none;

        float t = glm::dot(ac, qvec) * inv_det;

        if (t < 0) return boost::none;

        // Now that we know an intersection occurs, we need to use u and v to perform barycentric
        // interpolation to find the correct attribute values
        return Intersection(
            r.origin() + t * r.direction(),
            glm::normalize((1 - u - v) * a.normal + u * b.normal + v * c.normal),
            (1 - u - v) * a.texcoord + u * b.texcoord + v * c.texcoord,
            nullptr,
            t
        );
    }

    boost::optional<Intersection> TriMesh::find_intersection(const Ray& r) const {
        float depth = std::numeric_limits<float>::infinity();
        boost::optional<Intersection> intersection;

        this->m_bvh.search(r, [&](auto& n) {
            for (Triangle* t : n.objects) {
                auto new_intersection = this->find_intersection(r, *t);

                if (new_intersection && new_intersection->distance() < depth) {
                    depth = new_intersection->distance();
                    intersection = new_intersection;
                }
            }
        });

        return intersection;
    }

    class TriMeshLoader {
        std::vector<Vertex> m_vertices;
        std::vector<Triangle> m_triangles;

        std::map<std::tuple<unsigned int, unsigned int, unsigned int>, unsigned int> m_vertex_indices;

        std::vector<glm::vec3> m_pos;
        std::vector<glm::vec2> m_tex;
        std::vector<glm::vec3> m_norm;

        unsigned int add_vertex(unsigned int pos, unsigned int tex, unsigned int norm);
        unsigned int add_vertex(std::string spec);
    public:
        TriMeshLoader() {}
        TriMeshLoader(const TriMeshLoader& other) = delete;

        TriMeshLoader& operator =(const TriMeshLoader& other) = delete;

        void handle_line(std::string line);
        std::shared_ptr<TriMesh> finish();
    };

    unsigned int TriMeshLoader::add_vertex(unsigned int pos, unsigned int tex, unsigned int norm) {
        auto t = std::make_tuple(pos, tex, norm);
        auto existing_entry = this->m_vertex_indices.find(t);

        if (existing_entry != this->m_vertex_indices.end()) {
            return existing_entry->second;
        }

        if (pos >= this->m_pos.size() || tex >= this->m_tex.size() || norm >= this->m_norm.size()) {
            throw std::runtime_error("Face vertex index out of range");
        }

        this->m_vertices.push_back(Vertex {
            .pos = this->m_pos[pos],
            .normal = this->m_norm[norm],
            .texcoord = this->m_tex[tex]
        });
        this->m_vertex_indices.emplace(t, this->m_vertices.size() - 1);

        return this->m_vertices.size() - 1;
    }

    unsigned int TriMeshLoader::add_vertex(std::string spec) {
        std::vector<std::string> parts;

        boost::split(parts, spec, boost::is_any_of("/"));

        if (parts.size() != 3) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Invalid face vertex specification \"" << spec << "\"";

                return ss.str();
            })());
        }

        int pos = std::stoi(parts[0]);
        if (pos < 0) {
            pos += this->m_pos.size() + 1;

            if (pos <= 0) {
                throw std::out_of_range("Negative index out of range");
            }
        } else if (pos == 0) {
            throw std::out_of_range("0 is not a valid index");
        }

        int tex = std::stoi(parts[1]);
        if (tex < 0) {
            tex += this->m_tex.size() + 1;

            if (tex <= 0) {
                throw std::out_of_range("Negative index out of range");
            }
        } else if (tex == 0) {
            throw std::out_of_range("0 is not a valid index");
        }

        int norm = std::stoi(parts[2]);
        if (norm < 0) {
            norm += this->m_norm.size() + 1;

            if (norm <= 0) {
                throw std::out_of_range("Negative index out of range");
            }
        } else if (norm == 0) {
            throw std::out_of_range("0 is not a valid index");
        }

        return this->add_vertex(
            static_cast<unsigned int>(pos - 1),
            static_cast<unsigned int>(tex - 1),
            static_cast<unsigned int>(norm - 1)
        );
    }

    static glm::vec3 parse_vec3(const std::vector<std::string>& parts, int start) {
        return glm::vec3(
            std::stof(parts[start]),
            std::stof(parts[start + 1]),
            std::stof(parts[start + 2])
        );
    }

    static glm::vec2 parse_vec2(const std::vector<std::string>& parts, int start) {
        return glm::vec2(
            std::stof(parts[start]),
            std::stof(parts[start + 1])
        );
    }

    void TriMeshLoader::handle_line(std::string line) {
        size_t comment_pos = line.find("#");

        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        boost::trim(line);

        if (line.size() == 0) {
            return;
        }

        std::vector<std::string> parts;

        boost::split(parts, line, boost::is_any_of(" \t"), boost::token_compress_on);

        if (parts[0] == "v") {
            if (parts.size() != 4) {
                throw std::runtime_error("Wrong number of arguments for \"v\"");
            }

            this->m_pos.push_back(parse_vec3(parts, 1));
        } else if (parts[0] == "vt") {
            if (parts.size() != 3) {
                throw std::runtime_error("Wrong number of arguments for \"vt\"");
            }

            auto v = parse_vec2(parts, 1);

            this->m_tex.push_back(glm::vec2(v.x, 1 - v.y));
        } else if (parts[0] == "vn") {
            if (parts.size() != 4) {
                throw std::runtime_error("Wrong number of arguments for \"vn\"");
            }

            this->m_norm.push_back(parse_vec3(parts, 1));
        } else if (parts[0] == "f") {
            if (parts.size() != 4) {
                throw std::runtime_error("Wrong number of arguments for \"f\"");
            }

            this->m_triangles.push_back(Triangle {
                .a = this->add_vertex(parts[1]),
                .b = this->add_vertex(parts[2]),
                .c = this->add_vertex(parts[3])
            });
        }
    }

    std::shared_ptr<TriMesh> TriMeshLoader::finish() {
        auto mesh = std::make_shared<TriMesh>(
            std::move(this->m_vertices),
            std::move(this->m_triangles)
        );

        this->m_vertex_indices.clear();
        this->m_pos.clear();
        this->m_tex.clear();
        this->m_norm.clear();

        return std::move(mesh);
    }

    std::shared_ptr<TriMesh> TriMesh::load_mesh(boost::filesystem::path path) {
        boost::filesystem::ifstream f(path);

        if (!f) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Failed to open object file \"" << path.string() << "\"";

                return ss.str();
            })());
        }

        TriMeshLoader loader;
        std::string line;

        while (std::getline(f, line)) {
            loader.handle_line(line);
        }

        if (f.bad()) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Error reading object file \"" << path.string() << "\"";

                return ss.str();
            })());
        }

        return loader.finish();
    }

    boost::optional<Intersection> TriMeshObject::find_intersection(const Ray& r) const {
        auto intersection = this->m_mesh->find_intersection(r);

        if (intersection) {
            intersection->material(this->material());
        }

        return intersection;
    }
}
