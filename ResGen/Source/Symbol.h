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

using SymList = std::unordered_set<std::string>;
using SymMap = std::unordered_map<std::string, std::string>;

class InvalidRuleException final : public std::runtime_error {
public:
    InvalidRuleException(std::string_view rule) : std::runtime_error("invalid rule \"" + std::string(rule) + "\"") {}
};

class SymRule final {
public:
    constexpr static std::size_t FLAG_EXCLUDE = 0x01;
    constexpr static std::size_t FLAG_PARTIAL_MATCH = 0x02;

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
};

class SymDefs final {
    std::unordered_set<std::string> m_RuleNames;
    std::vector<SymRule> m_Rules;

public:
    SymDefs() {}

    bool parse(const std::filesystem::path& path);

    void clear() {
        m_RuleNames.clear();
        m_Rules.clear();
    }

    std::optional<std::string> resolve(std::string_view sym) const {
        // Find a rule for this symbol.
        std::vector<std::string_view> patterns;
        const SymRule* matched = nullptr;
        for (const auto& rule : m_Rules) {
            if (!rule.match(sym))
                continue;

            if (!matched)
                matched = &rule;

            patterns.push_back(rule.pattern());
        }

        if (patterns.size() > 1) {
            resgen::printWarning({}, "multiple rules match symbol \"{}\"", sym);

            for (const auto& pattern : patterns)
                resgen::printNote("matches \"{}\"", pattern);
        }

        // Return the same name if no rule matched.
        return matched ? matched->nameForSymbol(sym) : std::string(sym);
    }
};

bool parseSymInput(const std::filesystem::path& path, SymList& out);

} // namespace resgen

#endif /* _RESGEN_SYMBOL_H */