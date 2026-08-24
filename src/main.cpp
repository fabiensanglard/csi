#include "commands/fill_command.h"
#include "commands/lava_command.h"
#include "commands/palette256_command.h"

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct GlobalArgs {
    ColorMode colorMode = ColorMode::Rgb;
    bool help = false;
    bool version = false;
    std::vector<char*> rest;
};

// Consumes the options main owns and collects everything else for the command.
GlobalArgs parseGlobalArgs(int argc, char* argv[]) {
    GlobalArgs args;
    for (int argumentIndex = 1; argumentIndex < argc; ++argumentIndex) {
        const std::string argument = argv[argumentIndex];
        if (argument == "-h" || argument == "--help") {
            args.help = true;
        } else if (argument == "-v" || argument == "--version") {
            args.version = true;
        } else if (argument == "--space") {
            if (argumentIndex + 1 >= argc) {
                throw std::invalid_argument("missing value for " + argument);
            }
            args.colorMode = parseColorMode(argv[++argumentIndex]);
        } else {
            args.rest.push_back(argv[argumentIndex]);
        }
    }
    return args;
}

std::unique_ptr<Command> createCommand(const std::string& name) {
    if (name == "fill") {
        return std::make_unique<FillCommand>();
    }
    if (name == "lava") {
        return std::make_unique<LavaCommand>();
    }
    if (name == "256") {
        return std::make_unique<Palette256Command>();
    }
    return nullptr;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const GlobalArgs globalArgs = parseGlobalArgs(argc, argv);
        const std::unique_ptr<Terminal> terminal = createTerminal(globalArgs.colorMode);
        os::installInterruptHandler();

        if (globalArgs.version) {
            terminal->writeText("csi 1.0.0\n");
            terminal->commit();
            return 0;
        }

        if (globalArgs.help) {
            printUsage(*terminal, argv[0]);
            return 0;
        }

        if (globalArgs.rest.empty()) {
            printUsage(*terminal, argv[0]);
            return 2;
        }

        const std::unique_ptr<Command> command = createCommand(globalArgs.rest.front());
        if (!command) {
            throw std::invalid_argument(std::string("unknown command: ") + globalArgs.rest.front());
        }

        std::vector<char*> commandArgs(globalArgs.rest.begin() + 1, globalArgs.rest.end());
        command->parseArgs(static_cast<int>(commandArgs.size()), commandArgs.data());
        const int status = command->run(*terminal);

        // Hold the rendered screen so it is not scrolled away by the shell prompt.
        // A command that already stopped on Ctrl-C, and one that failed, both fall
        // through: the interrupt is spent, and an error should not hang.
        if (status == 0) {
            os::waitForInterrupt();
        }
        return status;
    } catch (const std::exception& error) {
        os::write(std::string("Error: ") + error.what() + "\n");
        return 2;
    }
}
