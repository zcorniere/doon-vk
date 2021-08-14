#include <exception>
#include <iostream>
#include <memory>
#include <stdlib.h>

#include "Application.hpp"
#include "Logger.hpp"
#include "types/VulkanException.hpp"

Logger *logger = nullptr;

__attribute__((constructor)) void ctor()
{
    logger = new Logger(std::cout);
    logger->start();
}
__attribute__((destructor)) void dtor() { delete logger; }

int main()
try {
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
} catch (const std::exception &e) {
    logger->err("EXCEPTION") << e.what();
    logger->endl();
    return EXIT_FAILURE;
}
