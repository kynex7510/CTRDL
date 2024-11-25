#ifndef _RESGEN_TOOLCTX_H
#define _RESGEN_TOOLCTX_H

#include "SymParse.h"
#include "CmdArgs.h"

#include <unordered_set>
#include <string_view>
#include <string>

namespace resgen {

class ToolCtx {
    SymMap m_SymsDefs;
    std::unordered_set<std::string> m_ExcludedSyms;
    std::string m_ResolverName;

public:
    bool init(const CmdArgs& args);
    bool isExcluded(std::string_view sym) const { return m_ExcludedSyms.contains(std::string(sym)); }

    std::string symbolDefinition(std::string_view sym) const {
        auto s = std::string(sym);

        if (auto it = m_SymsDefs.find(s); it != m_SymsDefs.end())
            return it->second;

        return s;
    }

    const std::string& resolverName() const { return m_ResolverName; }
};

} // namespace resgen

#endif /* _RESGEN_TOOLCTX_H */