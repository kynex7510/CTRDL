#ifndef _RESGEN_SYMPARSE_H
#define _RESGEN_SYMPARSE_H

#include "Print.h"

#include <unordered_map>
#include <unordered_set>
#include <filesystem>

namespace resgen {

using SymMap = std::unordered_map<std::string, std::string>;
using SymList = std::unordered_set<std::string>;

class SymMapBuilder {
    std::filesystem::path m_LastPath;
    SymMap m_SymMap;
    std::size_t m_CurrentLine = 0;
    std::size_t m_CurrentColumn = 0;

    template <typename ... Args>
    void printWarning(std::string_view fmt, Args&&... args) {
        resgen::printWarning(PrintFileInfo {
            .fileName = lastFileName(),
            .line = m_CurrentLine,
            .column = m_CurrentColumn,
            .boldText = true,
        }, fmt, std::forward<Args>(args)...);
    }

    template <typename ... Args>
    void printError(std::string_view fmt, Args&&... args) {
        resgen::printError(PrintFileInfo {
            .fileName = lastFileName(),
            .line = m_CurrentLine,
            .column = m_CurrentColumn,
            .boldText = true,
        }, fmt, std::forward<Args>(args)...);
    }

public:
    bool parse(const std::filesystem::path& path);

    const std::filesystem::path& lastPath() const { return m_LastPath; }
    std::string lastFileName() const { return lastPath().filename().string(); }
    SymMap&& extractSymMap() { return std::move(m_SymMap); }

    SymList extractSymList() {
        auto&& symMap = extractSymMap();

        SymList symList;
        for (const auto& [sym, _] : symMap)
            symList.insert(sym);

        return symList;
    }
};

bool parseSymList(const std::filesystem::path& path, SymList& out);

} // namespace resgen

#endif /* _RESGEN_SYMPARSE_H */