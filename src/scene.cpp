#include <map>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "scene.hpp"

namespace hw4 {
    class SceneLoader {
        Scene* m_scene;
        std::istream* m_stream;
        boost::filesystem::path m_dir;

        std::map<std::string, std::shared_ptr<TriMesh>> m_models;
        std::map<std::string, std::shared_ptr<Material>> m_materials;

        size_t m_current_line_number = 0;
        std::vector<std::string> m_current_line;
        size_t m_current_indent;

        bool read_next_line();
        std::runtime_error syntax_error(std::function<void (std::ostream&)> fn);
        boost::filesystem::path resolve_path(std::string path);

        glm::vec3 read_vec3(size_t offset);

        glm::vec3 parse_vec3_attr(const std::string& name);
        float parse_float_attr(const std::string& name);
        std::string parse_string_attr(const std::string& name);
        boost::filesystem::path parse_path_attr(const std::string& name);

        void parse_mdl();
        void parse_mtl();
        void parse_alight();
        void parse_plight();
        void parse_obj_mesh();
        void parse_obj_sphere();
        void parse_camera();
    public:
        SceneLoader(
            Scene* scene,
            std::istream* stream,
            boost::filesystem::path dir
        ) : m_scene(scene), m_stream(stream), m_dir(dir) {}

        void load();
    };

    static size_t count_indentation(const std::string& line) {
        size_t indentation = 0;

        for (char c : line) {
            if (c == ' ') {
                indentation += 1;
            } else if (c == '\t') {
                indentation += 4;
            } else {
                break;
            }
        }

        return indentation;
    }

    bool SceneLoader::read_next_line() {
        std::string line;

        while (std::getline(*this->m_stream, line)) {
            size_t comment_pos = line.find("#");

            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }

            size_t indent = count_indentation(line);
            boost::trim(line);

            this->m_current_line_number++;

            if (line.size() > 0) {
                this->m_current_line.clear();
                boost::split(
                    this->m_current_line,
                    line,
                    boost::is_any_of(" \t"),
                    boost::token_compress_on
                );

                this->m_current_indent = indent;
                return true;
            }
        }

        this->m_current_line.clear();
        this->m_current_indent = 0;
        return false;
    }

    std::runtime_error SceneLoader::syntax_error(std::function<void (std::ostream&)> fn) {
        std::ostringstream ss;

        ss << "Syntax error on line " << this->m_current_line_number << ": ";
        fn(ss);

        return std::runtime_error(ss.str());
    }

    boost::filesystem::path SceneLoader::resolve_path(std::string path) {
        boost::filesystem::path p(path);

        if (p.is_absolute()) {
            return p;
        } else {
            return this->m_dir / p;
        }
    }

    glm::vec3 SceneLoader::read_vec3(size_t offset) {
        assert(this->m_current_line.size() >= offset + 2);

        return glm::vec3(
            std::stof(this->m_current_line[offset]),
            std::stof(this->m_current_line[offset + 1]),
            std::stof(this->m_current_line[offset + 2])
        );
    }

    glm::vec3 SceneLoader::parse_vec3_attr(const std::string& name) {
        if (this->m_current_line.size() != 4) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Wrong number of arguments for " << name << " attribute";
            });
        }

        try {
            return this->read_vec3(1);
        } catch (std::exception& e) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Invalid arguments for " << name << " attribute";
            });
        }
    }

    float SceneLoader::parse_float_attr(const std::string& name) {
        if (this->m_current_line.size() != 2) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Wrong number of arguments for " << name << " attribute";
            });
        }

        try {
            return std::stof(this->m_current_line[1]);
        } catch (std::exception& e) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Invalid arguments for " << name << " attribute";
            });
        }
    }

    std::string SceneLoader::parse_string_attr(const std::string& name) {
        if (this->m_current_line.size() != 2) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Wrong number of arguments for " << name << " attribute";
            });
        }

        return this->m_current_line[1];
    }

    boost::filesystem::path SceneLoader::parse_path_attr(const std::string& name) {
        return this->resolve_path(this->parse_string_attr(name));
    }

    void SceneLoader::parse_mdl() {
        if (this->m_current_line.size() != 3) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Wrong number of arguments for mdl command";
            });
        }

        if (this->m_models.find(this->m_current_line[1]) != this->m_models.end()) {
            throw this->syntax_error([&](auto& ss) {
                ss << "A model \"" << this->m_current_line[1] << "\" already exists";
            });
        }

        this->m_models[this->m_current_line[1]] = TriMesh::load_mesh(
            this->resolve_path(this->m_current_line[2])
        );

        this->read_next_line();
    }

    void SceneLoader::parse_mtl() {
        if (this->m_current_line.size() != 2) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Wrong number of arguments for mtl command";
            });
        }

        if (this->m_materials.find(this->m_current_line[1]) != this->m_materials.end()) {
            throw this->syntax_error([&](auto& ss) {
                ss << "A material \"" << this->m_current_line[1] << "\" already exists";
            });
        }

        auto name = this->m_current_line[1];
        size_t indent = this->m_current_indent;

        auto ambient = glm::vec3(1);
        auto diffuse = glm::vec3(1);
        auto specular = glm::vec3(1);
        float shininess = 1;

        std::shared_ptr<Texture2D> diffuse_texture;
        std::shared_ptr<Texture2D> ao_texture;

        float reflectance = 0;

        if (this->read_next_line() && this->m_current_indent > indent) {
            indent = this->m_current_indent;

            do {
                const auto& cmd = this->m_current_line[0];

                if (cmd == "ambient") {
                    ambient = this->parse_vec3_attr("mtl::ambient");
                } else if (cmd == "diffuse") {
                    diffuse = this->parse_vec3_attr("mtl::diffuse");
                } else if (cmd == "specular") {
                    specular = this->parse_vec3_attr("mtl::specular");
                } else if (cmd == "shininess") {
                    shininess = this->parse_float_attr("mtl::shininess");
                } else if (cmd == "reflectance") {
                    reflectance = this->parse_float_attr("mtl::reflectance");
                } else if (cmd == "diffuse_map") {
                    diffuse_texture = Texture2D::load(this->parse_path_attr("mtl::diffuse_map").string());
                } else if (cmd == "ao_map") {
                    ao_texture = Texture2D::load(this->parse_path_attr("mtl::ao_map").string());
                } else {
                    throw this->syntax_error([&](auto& ss) {
                        ss << "Invalid mtl attribute \"" << cmd << "\"";
                    });
                }
            } while(this->read_next_line() && this->m_current_indent == indent);
        }

        this->m_materials[name] = std::make_shared<Material>(
            Material::textured(
                Material::reflective(
                    Material::diffuse(ambient, diffuse, specular, shininess),
                    reflectance
                ),
                diffuse_texture,
                ao_texture
            )
        );
    }

    void SceneLoader::parse_alight() {
        // TODO Implement this
        throw this->syntax_error([&](auto& ss) {
            ss << "Unimplemented command: alight";
        });
    }

    void SceneLoader::parse_plight() {
        if (this->m_current_line.size() != 1) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Wrong number of arguments for plight command";
            });
        }

        size_t indent = this->m_current_indent;

        auto pos = glm::vec3(0);
        auto ambient = glm::vec3(0);
        auto diffuse = glm::vec3(0);
        auto specular = glm::vec3(0);
        auto attenuation = glm::vec3(0);

        // TODO Handle texture mapping

        if (this->read_next_line() && this->m_current_indent > indent) {
            indent = this->m_current_indent;

            do {
                const auto& cmd = this->m_current_line[0];

                if (cmd == "pos") {
                    pos = this->parse_vec3_attr("plight::pos");
                } else if (cmd == "ambient") {
                    ambient = this->parse_vec3_attr("plight::ambient");
                } else if (cmd == "diffuse") {
                    diffuse = this->parse_vec3_attr("plight::diffuse");
                } else if (cmd == "specular") {
                    specular = this->parse_vec3_attr("plight::specular");
                } else if (cmd == "atten") {
                    attenuation = this->parse_vec3_attr("plight::atten");
                } else {
                    throw this->syntax_error([&](auto& ss) {
                        ss << "Invalid plight attribute \"" << cmd << "\"";
                    });
                }
            } while (this->read_next_line() && this->m_current_indent == indent);
        }

        this->m_scene->point_lights().push_back(std::make_unique<PointLight>(
            pos,
            ambient,
            diffuse,
            specular,
            attenuation
        ));
    }

    void SceneLoader::parse_obj_mesh() {
        size_t indent = this->m_current_indent;

        std::shared_ptr<TriMesh> mdl;
        std::shared_ptr<Material> mtl;

        glm::vec3 pos;
        glm::vec3 rot;
        float scale = 1.0f;

        if (this->read_next_line() && this->m_current_indent > indent) {
            indent = this->m_current_indent;

            do {
                const auto& cmd = this->m_current_line[0];

                if (cmd == "mdl") {
                    auto mdl_name = this->parse_string_attr("obj::mdl");
                    auto mdl_it = this->m_models.find(mdl_name);

                    if (mdl_it == this->m_models.end()) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "No such model: " << this->m_current_line[1];
                        });
                    }

                    mdl = mdl_it->second;
                } else if (cmd == "mtl") {
                    auto mtl_name = this->parse_string_attr("obj::mtl");
                    auto mtl_it = this->m_materials.find(this->m_current_line[1]);

                    if (mtl_it == this->m_materials.end()) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "No such material: " << this->m_current_line[1];
                        });
                    }

                    mtl = mtl_it->second;
                } else if (cmd == "pos") {
                    pos = this->parse_vec3_attr("pos");
                } else if (cmd == "rot") {
                    rot = this->parse_vec3_attr("rot");
                } else if (cmd == "scale") {
                    scale = this->parse_float_attr("obj::scale");
                } else {
                    throw this->syntax_error([&](auto& ss) {
                        ss << "Invalid obj attribute \"" << cmd << "\"";
                    });
                }
            } while (this->read_next_line() && this->m_current_indent == indent);
        }

        if (!mtl) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Attribute obj::mtl is required";
            });
        } else if (!mdl) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Attribute obj::mdl is required for mesh objects";
            });
        }

        this->m_scene->objects().push_back(std::make_unique<TriMeshObject>(
            mdl,
            apply_orientation(
                glm::scale(
                    glm::translate(glm::mat4(), pos),
                    glm::vec3(scale)
                ),
                rot
            ),
            mtl
        ));
    }

    void SceneLoader::parse_obj_sphere() {
        size_t indent = this->m_current_indent;

        std::shared_ptr<Material> mtl;

        glm::vec3 pos;
        float scale = 1.0f;

        if (this->read_next_line() && this->m_current_indent > indent) {
            indent = this->m_current_indent;

            do {
                const auto& cmd = this->m_current_line[0];

                if (cmd == "mtl") {
                    auto mtl_name = this->parse_string_attr("obj::mtl");
                    auto mtl_it = this->m_materials.find(this->m_current_line[1]);

                    if (mtl_it == this->m_materials.end()) {
                        throw this->syntax_error([&](auto& ss) {
                            ss << "No such material: " << this->m_current_line[1];
                        });
                    }

                    mtl = mtl_it->second;
                } else if (cmd == "pos") {
                    pos = this->parse_vec3_attr("obj::pos");
                } else if (cmd == "scale") {
                    scale = this->parse_float_attr("obj::scale");
                } else if (cmd == "mdl" || cmd == "rot") {
                    throw this->syntax_error([&](auto& ss) {
                        ss << "Attribute obj::" << cmd << " is not valid for sphere objects";
                    });
                } else {
                    throw this->syntax_error([&](auto& ss) {
                        ss << "Invalid obj attribute \"" << cmd << "\"";
                    });
                }
            } while (this->read_next_line() && this->m_current_indent == indent);
        }

        if (!mtl) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Attribute obj::mtl is required";
            });
        }

        this->m_scene->objects().push_back(std::make_unique<SphereObject>(
            scale,
            pos,
            mtl
        ));
    }

    void SceneLoader::parse_camera() {
        if (this->m_current_line.size() != 2) {
            throw this->syntax_error([&](auto& ss) {
                ss << "Wrong number of arguments for cam command";
            });
        }

        if (this->m_scene->cameras().find(this->m_current_line[1]) != this->m_scene->cameras().end()) {
            throw this->syntax_error([&](auto& ss) {
                ss << "A camera \"" << this->m_current_line[1] << "\" already exists";
            });
        }

        auto name = this->m_current_line[1];
        size_t indent = this->m_current_indent;

        auto pos = glm::vec3(0, 0, 1);
        auto look_at = glm::vec3(0, 0, 0);
        auto up = glm::vec3(0, 1, 0);
        float hfov = 90.0f;

        if (this->read_next_line() && this->m_current_indent > indent) {
            indent = this->m_current_indent;

            do {
                const auto& cmd = this->m_current_line[0];

                if (cmd == "pos") {
                    pos = this->parse_vec3_attr("cam::pos");
                } else if (cmd == "lookat") {
                    look_at = this->parse_vec3_attr("cam::lookat");
                } else if (cmd == "up") {
                    up = this->parse_vec3_attr("cam::up");
                } else if (cmd == "hfov") {
                    hfov = this->parse_float_attr("cam::hfov");
                } else {
                    throw this->syntax_error([&](auto& ss) {
                        ss << "Invalid cam attribute \"" << cmd << "\"";
                    });
                }
            } while (this->read_next_line() && this->m_current_indent == indent);
        }

        this->m_scene->cameras().emplace(name, Camera(pos, look_at, up, glm::radians(hfov)));
    }

    void SceneLoader::load() {
        if (!this->read_next_line()) {
            return;
        }

        while (this->m_current_line.size() > 0) {
            const auto& cmd = this->m_current_line[0];

            if (this->m_current_indent != 0) {
                throw this->syntax_error([&](auto& ss) {
                    ss << "Bad indentation";
                });
            }

            if (cmd == "mdl") {
                this->parse_mdl();
            } else if (cmd == "mtl") {
                this->parse_mtl();
            } else if (cmd == "alight") {
                this->parse_alight();
            } else if (cmd == "plight") {
                this->parse_plight();
            } else if (cmd == "obj") {
                if (this->m_current_line.size() != 2) {
                    throw this->syntax_error([&](auto& ss) {
                        ss << "Wrong number of arguments for obj command";
                    });
                }

                auto type = this->m_current_line[1];

                if (type == "mesh") {
                    this->parse_obj_mesh();
                } else if (type == "sphere") {
                    this->parse_obj_sphere();
                } else {
                    throw this->syntax_error([&](auto& ss) {
                        ss << "Invalid object type \"" << type << "\"";
                    });
                }
            } else if (cmd == "cam") {
                this->parse_camera();
            } else {
                throw this->syntax_error([&](auto& ss) {
                    ss << "Invalid scene command \"" << cmd << "\"";
                });
            }
        }
    }

    Scene Scene::load_scene(boost::filesystem::path path) {
        boost::filesystem::ifstream f(path);

        if (!f) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Failed to open object file \"" << path.string() << "\"";

                return ss.str();
            })());
        }

        Scene scene;
        SceneLoader loader(&scene, &f, path.parent_path());

        loader.load();

        if (f.bad()) {
            throw std::runtime_error(([&]() {
                std::ostringstream ss;

                ss << "Error reading scene file \"" << path.string() << "\"";

                return ss.str();
            })());
        }

        return scene;
    }
}
