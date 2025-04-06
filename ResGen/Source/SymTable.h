#ifndef _RESGEN_SYMTABLE_H
#define _RESGEN_SYMTABLE_H

#include "Symbol.h"

#include <vector>
#include <string>
#include <unordered_map>

namespace resgen {

class SymTable {
    std::vector<std::uint8_t> m_StringTable;
    std::vector<std::vector<size_t>> m_Buckets;
    std::unordered_map<size_t, SymEntry> m_Mapped;

    size_t insertSymbol(const std::string& sym, const SymEntry& mapped);
    void genHashTable(SymMap&& symbols, std::unordered_map<std::string, size_t>&& offsets);

public:
    SymTable(SymMap&& symbols);

    const std::vector<std::uint8_t>& stringTable() const { return m_StringTable; }
    const std::vector<std::vector<size_t>>& buckets() const { return m_Buckets; }
    const SymEntry& mapped(size_t idx) const { return m_Mapped.at(idx); }
};

} // namespace resgen

#endif /* _RESGEN_SYMTABLE_H */