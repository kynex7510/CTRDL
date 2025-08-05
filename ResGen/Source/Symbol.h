#ifndef _RESGEN_SYMBOL_H
#define _RESGEN_SYMBOL_H

#include <re2/re2.h>

#include "Print.h"

#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace resgen {

struct SymEntry {
    struct Hash {
        std::size_t operator()(const SymEntry& other) const noexcept { return std::hash<std::string>{}(other.name); }
    };

    std::string name;
    bool isWeak = false;

    bool operator==(const SymEntry& other) const noexcept { return name == other.name; }
};

using SymList = std::unordered_set<SymEntry, SymEntry::Hash>;
using SymMap = std::unordered_map<std::string, SymEntry>;

class InvalidRuleException final : public std::runtime_error {
public:
    InvalidRuleException(const std::string& pattern) : std::runtime_error("invalid rule \"" + pattern + "\"") {}
};

class SymRule final {
public:
    constexpr static std::size_t FLAG_WEAK = 0x01;
    constexpr static std::size_t FLAG_EXCLUDE = 0x02;
    constexpr static std::size_t FLAG_PARTIAL_MATCH = 0x04;

private:
    std::unique_ptr<RE2> m_RE;
    std::optional<std::string> m_Name;
    std::size_t m_Flags;

public:
    SymRule(std::string_view rule, std::optional<std::string>&& name, std::size_t flags = 0u) : m_RE(std::make_unique<RE2>(rule)), m_Name(std::move(name)), m_Flags(flags) {
        if (!m_RE->ok())
            throw InvalidRuleException(rule);
    }

    SymRule(SymRule&&) noexcept = default;
    SymRule& operator=(SymRule&&) noexcept = default;

    std::string_view pattern() const { return m_RE->pattern(); }
    bool match(std::string_view s) const;

    std::optional<std::string> nameForSymbol(std::string_view sym) const {
        if (m_Flags & FLAG_EXCLUDE)
            return {};

        if (m_Name)
            return m_Name;

        return std::string(sym);
    }

    bool isWeak() const { return m_Flags & FLAG_WEAK; }
};

class SymDefs final {
    std::unordered_set<std::string> m_RulePatterns;
    std::vector<SymRule> m_Rules;

public:
    SymDefs() {}

    bool parseObject(std::string_view fileName, std::string_view content);
    bool parseFile(const std::filesystem::path& path);

    void clear() {
        m_RulePatterns.clear();
        m_Rules.clear();
    }

    // Find a rule for this symbol.
    const SymRule* ruleForSymName(std::string_view sym) const {
        const SymRule* matched = nullptr;
        std::vector<const SymRule*> otherMatches;
        for (const auto& rule : m_Rules) {
            if (!rule.match(sym))
                continue;

            if (!matched) {
                matched = &rule;
            } else {
                otherMatches.push_back(&rule);
            }
        }

        if (!otherMatches.empty()) {
            resgen::printWarning({}, "multiple rules match symbol \"{}\"", sym);
            resgen::printNote("chosen \"{}\"", matched->pattern());

            for (const auto& rule : otherMatches)
                resgen::printNote("matches \"{}\"", rule->pattern());
        }

        return matched;
    }
};

bool parseSymInput(const std::filesystem::path& path, SymList& out);

} // namespace resgen

#endif /* _RESGEN_SYMBOL_H */