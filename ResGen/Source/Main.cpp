#include "CmdArgs.h"
#include "Symbol.h"
#include "ResGenerator.h"
#include "Print.h"

using namespace resgen;

int main(int argc, const char* const* argv) {
    // Parse arguments.
    CmdArgs args;

    try {
        if (!args.parse(argc, argv))
            return 0;
    } catch (const cxxopts::exceptions::exception& ex) {
        resgen::printError({}, "{}", ex.what());
        return 1;
    }

    if (args.inputs().empty()) {
        resgen::printError({}, "no input files");
        return 1;
    }

    auto outPath = args.output();
    if (outPath.empty())
        outPath = "resolver.s";

    // Parse inputs.
    SymList syms;
    for (const auto& input : args.inputs()) {
        if (!resgen::parseSymInput(input, syms))
            return 1;
    }

    // Parse symdefs.
    SymDefs defs;
    for (const auto& defFile : args.definitionLists()) {
        if (!defs.parse(defFile))
            return 1;
    }

    // Generate symbol map.
    SymMap symMap;

    for (const auto& sym : syms) {
        if (auto name = defs.resolve(sym))
            symMap[sym] = *name;
    }

    // Generate resolver.
    if (!ResGenerator(std::move(SymTable(std::move(symMap))), args.resolverName()).writeToFile(outPath))
        return 1;

    return 0;
}