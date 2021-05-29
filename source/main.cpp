#include "Application.hpp"
#include "Logger.hpp"

Logger *logger = nullptr;

__attribute__((constructor)) void ctor()
{
    logger = new Logger();
    logger->start();
}
__attribute__((destructor)) void dtor() { delete logger; }

int main()
try {
    Application app;

    app.run();
    return EXIT_SUCCESS;
} catch (const vk::SystemError &se) {
    logger->err("VULKAN_ERROR") << se.what();
    logger->endl();
    return EXIT_FAILURE;
} catch (const std::exception &e) {
    logger->err("EXCEPTION") << e.what();
    logger->endl();
    return EXIT_FAILURE;
}