#include <cxxopts.hpp>

#include "CmdArgs.h"
#include "Print.h"

using namespace resgen;

constexpr static char DEFAULT_RESOLVER_NAME[] = "ctrdlProgramResolver";

static void getListsInDirs(const std::vector<std::string>& dirs, std::unordered_set<std::filesystem::path>& lists) {
    for (const auto& dir : dirs) {
        if (!std::filesystem::exists(dir)) {
            resgen::printWarning({}, "directory \"{}\" doesn't exist", dir);
            continue;
        }

        if (!std::filesystem::is_directory(dir)) {
            resgen::printWarning({}, "\"{}\" is not a directory", dir);
            continue;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                const auto& path = entry.path();
                if (path.extension() == ".symlist")
                    lists.insert(path);
            }
        }
    }
}

CmdArgs::CmdArgs() : m_Options("ResGen", "Resolver Generator") {
    m_Options.add_options()
        ("h,help", "Show help")
        ("inputs", "Binary inputs", cxxopts::value<std::vector<std::string>>())
        ("d,define", "List containing symbol definitions", cxxopts::value<std::vector<std::string>>())
        ("e,exclude", "List containing symbols to be excluded", cxxopts::value<std::vector<std::string>>())
        ("D,define-dir", "Directory containing definition lists", cxxopts::value<std::vector<std::string>>())
        ("E,exclude-dir", "Directory containing exclusion lists", cxxopts::value<std::vector<std::string>>())
        ("n,name", "Set the symbol name for the resolver", cxxopts::value<std::string>()->default_value(DEFAULT_RESOLVER_NAME));

    m_Options.parse_positional("inputs");
}

bool CmdArgs::parse(int argc, const char* const* argv) {
    m_Inputs.clear();
    m_DefinitionLists.clear();
    m_ExclusionLists.clear();
    m_ResolverName.clear();

    auto result = m_Options.parse(argc, argv);

    if (result.count("help")) {
        resgen::print({}, {}, "{}", m_Options.help());
        return false;
    }

    if (result["inputs"].count()) {
        for (const auto& entry : result["inputs"].as<std::vector<std::string>>())
            m_Inputs.insert(entry);
    }

    if (result["d"].count()) {
        for (const auto& entry : result["d"].as<std::vector<std::string>>())
            m_DefinitionLists.insert(entry);
    }

    if (result["e"].count()) {
        for (const auto& entry : result["e"].as<std::vector<std::string>>())
            m_ExclusionLists.insert(entry);
    }

    if (result["D"].count())
        getListsInDirs(result["D"].as<std::vector<std::string>>(), m_DefinitionLists);

    if (result["E"].count())
        getListsInDirs(result["E"].as<std::vector<std::string>>(), m_ExclusionLists);
        
    m_ResolverName = result["n"].as<std::string>();
    return true;
}