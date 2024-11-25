#ifndef _RESGEN_RESGENERATOR_H
#define _RESGEN_RESGENERATOR_H

#include "SymTable.h"
#include "ToolCtx.h"

#include <filesystem>

namespace resgen {

class ResGenerator {
    SymTable m_SymTable;
    std::string m_ResolverName;

public:
    ResGenerator(const ToolCtx&, SymTable&&);
    void writeToFile(const std::filesystem::path& path);
};

} // namespace resgen

#endif /* _RESGEN_RESGENERATOR_H */