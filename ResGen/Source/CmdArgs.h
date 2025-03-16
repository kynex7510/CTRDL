#ifndef _RESGEN_CMDARGS_H
#define _RESGEN_CMDARGS_H

#include <cxxopts.hpp>

#include <filesystem>
#include <vector>
#include <unordered_set>

namespace resgen {

class CmdArgs {
    cxxopts::Options m_Options;
    std::unordered_set<std::filesystem::path> m_Inputs;
    std::filesystem::path m_Output;
    std::unordered_set<std::filesystem::path> m_DefinitionLists;
    std::string m_ResolverName;

public:
    CmdArgs();

    bool parse(int argc, const char* const* argv);

    const std::unordered_set<std::filesystem::path>& inputs() const { return m_Inputs; }
    const std::filesystem::path& output() const { return m_Output; }
    const std::unordered_set<std::filesystem::path>& definitionLists() const { return m_DefinitionLists; }
    const std::string& resolverName() const { return m_ResolverName; }
};

} // namespace resgen

#endif /* _RESGEN_CMDARGS_H */