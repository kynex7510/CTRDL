#include "CmdArgs.h"
#include "ToolCtx.h"
#include "SymTable.h"
#include "ResGenerator.h"
#include "Print.h"

using namespace resgen;

static void genResolver(const ToolCtx& toolCtx, const std::filesystem::path& path) {
    SymList symList;

    if (!resgen::parseSymList(path, symList))
        return;

    SymMap symMap;
    for (const auto& sym : symList) {
        if (toolCtx.isExcluded(sym))
            continue;

        symMap.insert({ sym, toolCtx.symbolDefinition(sym) });
    }

    auto outPath = path;
    outPath.replace_extension(".s");
    ResGenerator(toolCtx, std::move(SymTable(std::move(symMap)))).writeToFile(outPath);
}

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
        resgen::printError({}, "at least one input list must be specified");
        return 1;
    }

    // Create context.
    ToolCtx toolCtx;
    if (!toolCtx.init(args))
        return 1;

    // Generate resolvers.
    for (const auto& input : args.inputs())
        genResolver(toolCtx, input);

    return 0;
}