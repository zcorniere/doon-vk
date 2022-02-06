#include <pivot/graphics/pivot.hxx>
#include <iostream>

#include "Application.hpp"

struct CmdOption {
    unsigned bVerbose = 0;
};

// CmdOption getCmdLineOption(int ac, char **av)
// {
//     CmdOption opt{};
//     int c;

//     while ((c = getopt(ac, av, "v")) != -1) {
//         switch (c) {
//             case 'v': opt.bVerbose += 1; break;
//             default: break;
//         }
//     }

//     return opt;
// }

int main(int ac, char **av)
try{
    logger.start(Logger::Level::Info);

    CmdOption options = {};    // getCmdLineOption(ac, av);
    switch (options.bVerbose) {
        case 1: logger.setLevel(Logger::Level::Debug); break;
        case 2: logger.setLevel(Logger::Level::Trace); break;
        default: break;
    }

    Application app;
    app.init();
    app.run();
    return 0;
}
CATCH_PIVOT_EXCEPTIONS
