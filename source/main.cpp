#include "Application.hpp"
int main()
try {
    Application app;

    app.run();
    return EXIT_SUCCESS;
} catch (const std::exception &e) {
    return EXIT_FAILURE;
}