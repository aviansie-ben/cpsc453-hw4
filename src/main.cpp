#include <atomic>
#include <chrono>
#include <cstdlib>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "render.hpp"
#include "scene.hpp"

namespace hw4 {
    struct ProgramOptions {
        bool no_preview;
        int supersample_level;
        int max_recursion;
        float bias;
        glm::ivec2 size;

        boost::filesystem::path scene;
        std::string camera;
        boost::filesystem::path output;
    };

    void print_image(const Image& img, std::ostream& s) {
        for (int y = 0; y < img.size().y; y += 2) {
            for (int x = 0; x < img.size().x; x++) {
                auto pt = img.get_pixel(glm::ivec2(x, y));
                auto pb = y < img.size().y - 1 ? img.get_pixel(glm::ivec2(x, y + 1)) : glm::vec3(0);

                s << "\033[38;2;"
                  << static_cast<int>(pt.r * 255) << ";"
                  << static_cast<int>(pt.g * 255) << ";"
                  << static_cast<int>(pt.b * 255) << "m"
                  << "\033[48;2;"
                  << static_cast<int>(pb.r * 255) << ";"
                  << static_cast<int>(pb.g * 255) << ";"
                  << static_cast<int>(pb.b * 255) << "m";

                s << "▀";
            }

            s << "\033[0m\n";
        }

        s << std::flush;
    }

    void wait_with_spinner(std::future<void> f) {
        int state = 0;

        std::cout << " ";

        do {
            switch (state) {
            case 0:
                std::cout << "-";
                break;
            case 1:
                std::cout << "\\";
                break;
            case 2:
                std::cout << "|";
                break;
            case 3:
                std::cout << "/";
                break;
            }

            std::cout << "\033[1D" << std::flush;

            state++;
            if (state == 4) state = 0;
        } while (f.wait_for(std::chrono::milliseconds(250)) == std::future_status::timeout);

        try {
            f.get();
        } catch (std::exception& e) {
            std::cout << "\033[KFAIL\n" << e.what() << std::endl;
            std::exit(1);
        }

        std::cout << "DONE" << std::endl;
    }

    void wait_with_spinner_and_progress(
        std::future<void> f,
        const std::function<float ()>& progress_fn
    ) {
        int state = 0;

        std::cout << " ";

        do {
            std::ostringstream ss;

            switch (state) {
            case 0:
                ss << "-";
                break;
            case 1:
                ss << "\\";
                break;
            case 2:
                ss << "|";
                break;
            case 3:
                ss << "/";
                break;
            }

            ss << " ["
               << std::fixed << std::setprecision(2) << std::setw(6) << progress_fn() * 100
               << "%]";

            std::cout << "\033[K" << ss.str();
            std::cout << "\033[" << ss.str().size() << "D" << std::flush;

            state++;
            if (state == 4) state = 0;
        } while (f.wait_for(std::chrono::milliseconds(250)) == std::future_status::timeout);

        try {
            f.get();
        } catch (std::exception& e) {
            std::cout << "\033[KFAIL\n" << e.what() << std::endl;
            std::exit(1);
        }

        std::cout << "\033[KDONE" << std::endl;
    }

    void render_with_progress(
        const RayTraceRenderer& r,
        const Scene& s,
        Image& i
    ) {
        std::atomic<float> progress(0);

        wait_with_spinner_and_progress(
            std::async([&]() {
                i = r.render(s, [&](float p) {
                    progress.store(p);
                });
            }),
            [&]() {
                return progress.load();
            }
        );
    }

    void show_scene(const Scene& scene, const Camera& camera, const ProgramOptions& options) {
        RayTraceRenderer render(
            glm::ivec2(160, 120),
            options.max_recursion,
            2,
            options.bias,
            camera
        );
        Image img;

        std::cout << "Generating preview...";
        render_with_progress(
            render,
            scene,
            img
        );

        print_image(img, std::cout);
    }

    void render_scene(const Scene& scene, const Camera& camera, const ProgramOptions& options) {
        RayTraceRenderer render(
            options.size,
            options.max_recursion,
            options.supersample_level,
            options.bias,
            camera
        );
        Image img;

        std::cout << "Generating full-size image...";
        render_with_progress(
            render,
            scene,
            img
        );

        img.save_as_ppm(options.output);
    }

    struct option_ivec2 {
        glm::ivec2 v;

        option_ivec2() {}
        option_ivec2(glm::ivec2 v) : v(v) {}

        operator glm::ivec2() const { return this->v; }
    };

    std::ostream& operator <<(std::ostream& s, const option_ivec2& v) {
        return s << v.v.x << "," << v.v.y;
    }

    void validate(boost::any& v, const std::vector<std::string>& values, option_ivec2*, int) {
        namespace po = boost::program_options;

        po::validators::check_first_occurrence(v);
        const std::string& s = po::validators::get_single_string(values);
        std::vector<std::string> parts;

        boost::split(parts, s, boost::is_any_of(","));

        if (parts.size() != 2) {
            throw po::validation_error(po::validation_error::invalid_option_value);
        }

        try {
            v = option_ivec2(glm::ivec2(
                boost::lexical_cast<int>(parts[0]),
                boost::lexical_cast<int>(parts[1])
            ));
        } catch (std::exception& e) {
            throw po::validation_error(po::validation_error::invalid_option_value);
        }
    }

    boost::optional<ProgramOptions> parse_options(int argc, char** argv) {
        namespace po = boost::program_options;

        po::positional_options_description pos_options;
        pos_options.add("scene", 1);

        po::options_description hidden_options;
        hidden_options.add_options()
            ("scene", po::value<std::string>());

        po::options_description render_options("Render Options");
        render_options.add_options()
            ("no-preview", "Skip generating a preview image")
            (
                "camera,c",
                po::value<std::string>()
                    ->value_name("<camera name>")
                    ->default_value("default"),
                "Sets the name of the camera from the scene to use for rendering"
            )
            (
                "supersample",
                po::value<int>()
                    ->value_name("<n>")
                    ->default_value(2),
                "Use n^2 rays per pixel for supersampling"
            )
            (
                "max-recursion",
                po::value<int>()
                    ->value_name("<n>")
                    ->default_value(5),
                "Allow a maximum of n recursive rays to be traced (0 renders only view rays)"
            )
            (
                "bias",
                po::value<float>()
                    ->value_name("<v>")
                    ->default_value(1e-3f),
                "Use a bias of v units along the surface normal for recursive rays"
            )
            (
                "size,s",
                po::value<option_ivec2>()
                    ->value_name("<w>,<h>")
                    ->default_value(glm::ivec2(640, 480)),
                "The size (in pixels) of the image to generate"
            );

        po::options_description general_options("General Options");
        general_options.add_options()
            (
                ",o",
                po::value<std::string>()
                    ->value_name("<filename>")
                    ->default_value("output.ppm"),
                "The name of the PPM file to save the rendered image as"
            )
            ("help,h", "Display this help message");

        po::options_description all_options;
        all_options.add(hidden_options);
        all_options.add(render_options);
        all_options.add(general_options);

        po::variables_map vm;

        try {
            po::store(
                po::command_line_parser(argc, argv)
                    .options(all_options)
                    .positional(pos_options)
                    .run(),
                vm
            );
            po::notify(vm);
        } catch (po::error& e) {
            std::cerr << argv[0] << ": " << e.what() << "\nUse " << argv[0] << " -h for help\n";
            return boost::none;
        }

        if (vm.count("help")) {
            std::cerr << "Usage: " << argv[0] << " [options...] <scene>\n"
                      << "Simple Whitted Ray Tracer v0.1\n"
                      << render_options
                      << general_options;
            return boost::none;
        }

        if (!vm.count("scene")) {
            std::cerr << argv[0] << ": No scene file specified\nUse " << argv[0]
                      << " -h for help\n";
            return boost::none;
        }

        ProgramOptions result;

        result.no_preview = vm.count("no-preview") > 0;
        result.supersample_level = vm["supersample"].as<int>();
        result.max_recursion = vm["max-recursion"].as<int>();
        result.bias = vm["bias"].as<float>();
        result.size = vm["size"].as<option_ivec2>();

        result.scene = vm["scene"].as<std::string>();
        result.camera = vm["camera"].as<std::string>();
        result.output = vm["-o"].as<std::string>();

        return result;
    }

    extern "C" int main(int argc, char** argv) {
        auto options = parse_options(argc, argv);

        if (!options) {
            return 2;
        }

        Scene scene;
        Camera camera;

        std::cout << "Loading scene...";
        wait_with_spinner(std::async([&]() {
            scene = Scene::load_scene(options->scene);

            auto camera_it = scene.cameras().find(options->camera);

            if (camera_it == scene.cameras().end()) {
                throw std::runtime_error(([&]() {
                    std::ostringstream ss;

                    ss << "Scene does not contain a camera \"" << options->camera << "\"";

                    return ss.str();
                })());
            }

            camera = camera_it->second;
        }));

        std::cout << "Building mesh BVHs...";
        wait_with_spinner(std::async([&]() {
            scene.regen_mesh_bvhs(100);
        }));

        std::cout << "Building scene BVH...";
        wait_with_spinner(std::async([&]() {
            scene.regen_bvh(100);
        }));

        if (!options->no_preview) show_scene(scene, camera, *options);
        render_scene(scene, camera, *options);

        return 0;
    }
}
