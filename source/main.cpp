#include <pivot/graphics/pivot.hxx>
#include <iostream>

#include "Application.hpp"

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
try{
    logger.start(Logger::Level::Info);

    auto options = getCmdLineOption(ac, av);
    if (options.bVerbose) logger.setLevel(Logger::Level::Debug);

    Application app;
    app.init();
    app.run();
    return 0;
}
CATCH_PIVOT_EXCEPTIONS
