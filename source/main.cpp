#include <exception>
#include <iostream>
#include <memory>
#include <stdlib.h>

#include "Application.hpp"
#include "Logger.hpp"
#include "types/VulkanException.hpp"
#include <getopt.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

Logger *logger = nullptr;

__attribute__((constructor)) void ctor()
{
    logger = new Logger(std::cout);
    logger->start(Logger::Level::Info);
}
__attribute__((destructor)) void dtor() { delete logger; }

struct CmdOption {
    bool bVerbose = false;
};

CmdOption getCmdLineOption(int ac, char **av)
{
    CmdOption opt{};
    int c;

    while ((c = getopt(ac, av, "v")) != -1) {
        switch (c) {
            case 'v': opt.bVerbose = true; break;
            default: break;
        }
    }

    return opt;
}

int main(int ac, char **av)
try {
    CmdOption option = getCmdLineOption(ac, av);
    if (option.bVerbose) logger->setLevel(Logger::Level::Debug);

    Application app;

    app.init([&app]() {
        app.loadModel();
        app.loadTextures();
        return true;
    });

    app.run();
    return EXIT_SUCCESS;
} catch (const VulkanException &se) {
    logger->err("VULKAN_ERROR") << se.what();
    logger->endl();
    return EXIT_FAILURE;
} catch (const vk::SystemError &se) {
    logger->err("SYSTEM_ERROR") << se.what();
    logger->endl();
    return EXIT_FAILURE;
} catch (const std::exception &e) {
    logger->err("EXCEPTION") << e.what();
    logger->endl();
    return EXIT_FAILURE;
}
