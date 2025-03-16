#ifndef _RESGEN_RESGENERATOR_H
#define _RESGEN_RESGENERATOR_H

#include "SymTable.h"

#include <filesystem>

namespace resgen {

class ResGenerator {
    SymTable m_SymTable;
    std::string m_ResolverName;

public:
    ResGenerator(SymTable&& symTable, std::string_view resolverName);
    bool writeToFile(const std::filesystem::path& path);
};

} // namespace resgen

#endif /* _RESGEN_RESGENERATOR_H */