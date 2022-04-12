#include <iostream>

#include "Application.hpp"

struct CmdOption {
    unsigned bVerbose = 0;
};

CmdOption getCmdLineOption(int ac, char **av)
{
    CmdOption opt{};
    int c;
    while ((c = getopt(ac, av, "v")) != -1) {
        switch (c) {
            case 'v': opt.bVerbose += 1; break;
            default: break;
        }
    }
    return opt;
}

using Transform = pivot::graphics::Transform;

int main(int ac, char **av)
try{
    logger.start(Logger::Level::Info);

    CmdOption options = getCmdLineOption(ac, av);
    switch (options.bVerbose) {
        case 1: logger.setLevel(Logger::Level::Debug); break;
        case 2: logger.setLevel(Logger::Level::Trace); break;
        default: break;
    }

    Application app;

    for (unsigned i = 0; i < 5; i++) {
        for (unsigned y = 0; y < 5; y++) {
            app.scene.addObject({
                .meshID = "sponza",
                .pipelineID = ((y + i) % 2) ? ("wireframe") : (""),
                .transform =
                    {
                        .position = {i * 1600, 0, y * 1000},
                        .scale = glm::vec3(50),
                    },
            });
        }
    }

    app.scene.addObject({.meshID = "FlightHelmet",
                         .transform = {
                             .position = {0.f, 100.f, 0.f},
                             .scale = glm::vec3(100),
                         }});
    app.assetStorage.loadModels("sponza/sponza.gltf", "FlightHelmet/glTF/FlightHelmet.gltf");
    app.assetStorage.loadTextures("../textures/grey.png");
    app.init();
    app.run();
    return 0;
} catch (const pivot::PivotException &pe) {
    logger.err(pe.getScope()) << pe.getKind() << "/" << pe.what();
}
