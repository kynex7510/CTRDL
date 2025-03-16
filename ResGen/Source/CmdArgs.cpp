#include "CmdArgs.h"
#include "Print.h"

using namespace resgen;

constexpr static char DEFAULT_RESOLVER_NAME[] = "ctrdlProgramResolver";

CmdArgs::CmdArgs() : m_Options("ResGen", "Resolver Generator") {
    m_Options.add_options()
        ("h,help", "Show help")
        ("inputs", "", cxxopts::value<std::vector<std::string>>())
        ("o,output", "Output file", cxxopts::value<std::string>())
        ("rules", "JSON file(s) containing symbol definitions", cxxopts::value<std::vector<std::string>>())
        ("name", "Set the symbol name for the resolver", cxxopts::value<std::string>()->default_value(DEFAULT_RESOLVER_NAME));

    m_Options.parse_positional({ "inputs" });
}

bool CmdArgs::parse(int argc, const char* const* argv) {
    m_Inputs.clear();
    m_DefinitionLists.clear();
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

    if (result["output"].count())
        m_Output = result["output"].as<std::string>();

    if (result["rules"].count()) {
        for (const auto& entry : result["rules"].as<std::vector<std::string>>())
            m_DefinitionLists.insert(entry);
    }
        
    m_ResolverName = result["name"].as<std::string>();
    return true;
}