#include "SymTable.h"

#include <cstring>
#include <cmath>
#include <string_view>
#include <span>

using namespace resgen;

constexpr static std::uint32_t FNV_INIT = 0x811C9DC5;
constexpr static std::uint32_t FNV_PRIME = 0x01000193;

static std::uint32_t fnv(const std::span<const std::uint8_t> buffer) {
    std::uint32_t hash = FNV_INIT;

    for (auto i = 0u; i < buffer.size(); ++i) {
        hash ^= static_cast<std::uint32_t>(buffer[i]);
        hash *= FNV_PRIME;
    }

    return hash;
}

static std::uint32_t nearestPowerOf2(std::uint32_t n) {
    for (auto i = 0; i < sizeof(std::uint32_t) * 8; ++i) {
        const auto p = (1 << ((sizeof(std::uint32_t) * 8) - i - 1));
        if (p & n) {
            const auto lower = p;
            const auto upper = p << 1;
            return (upper - n) < (n - lower) ? upper : lower;
        }
    }

    return 0;
}

size_t SymTable::insertSymbol(const std::string& sym, const SymEntry& mapped) {
    const auto strOff = m_StringTable.size();
    m_StringTable.resize(strOff + sym.size() + 1, '\0');
    memcpy(m_StringTable.data() + strOff, sym.data(), sym.size());
    m_Mapped[strOff] = mapped;
    return strOff;
}

void SymTable::genHashTable(SymMap&& symbols, std::unordered_map<std::string, size_t>&& offsets) {
    m_Buckets.resize(nearestPowerOf2(std::sqrt(symbols.size())));

    for (const auto& [sym, mapped] : symbols) {
        const auto hash = fnv(std::span { reinterpret_cast<const std::uint8_t*>(sym.data()), sym.size() });
        auto& bucket = m_Buckets[hash % m_Buckets.size()];
        bucket.push_back(offsets[sym]);
    }
}

SymTable::SymTable(SymMap&& symbols) {
    std::unordered_map<std::string, size_t> offsets;

    for (const auto& [sym, mapped] : symbols)
        offsets.insert({ sym, insertSymbol(sym, mapped) });

    genHashTable(std::move(symbols), std::move(offsets));
}