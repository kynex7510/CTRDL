#include <nlohmann/json.hpp>

#include "Symbol.h"
#include "Print.h"

#include <fstream>

using namespace resgen;

bool SymRule::match(std::string_view s) const {
    if (m_Flags & FLAG_PARTIAL_MATCH)
        return RE2::PartialMatch(s, *m_RE);

    return RE2::FullMatch(s, *m_RE);
}

bool SymDefs::parseObject(std::string_view fileName, std::string_view content) {
    std::unordered_set<std::string> rulePatterns;
    std::vector<SymRule> rules;

    // Parse json.
    const auto jsonData = nlohmann::json::parse(content, nullptr, false);
    if (!jsonData.is_object()) {
        resgen::printError( PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "expected object");
        return false;
    }

    for (const auto& def : jsonData.items()) {
        // Ignore rule if already existing.
        const auto& pattern = def.key();
        if (rulePatterns.contains(pattern) || m_RulePatterns.contains(pattern)) {
            resgen::printWarning(PrintFileInfo {
                .fileName = fileName,
                .boldText = true,
            }, "duplicate rule \"{}\", ignored", pattern);

            if (m_RulePatterns.contains(pattern))
                resgen::printNote("declared in a different file");

            continue;
        }

        // Validate object.
        const auto& value = def.value();
        if (!value.is_object()) {
            resgen::printError( PrintFileInfo {
                .fileName = fileName,
                .boldText = true,
            }, "expected object for rule \"{}\"", pattern);
            return false;
        }

        // Extract informations.
        std::optional<std::string> name;
        std::size_t flags = 0;

        if (value.contains("name")) {
            if (!value["name"].is_string()) {
                resgen::printError( PrintFileInfo {
                    .fileName = fileName,
                    .boldText = true,
                }, "expected string for \"name\" of rule \"{}\"", pattern);
                return false;
            }

            name = value["name"].get<std::string>();
        }

        if (value.contains("weak")) {
            if (!value["weak"].is_boolean()) {
                resgen::printError( PrintFileInfo {
                    .fileName = fileName,
                    .boldText = true,
                }, "expected bool for \"weak\" of rule \"{}\"", pattern);
                return false;
            }

            if (value["weak"].get<bool>())
                flags |= SymRule::FLAG_WEAK;
        }

        if (value.contains("exclude")) {
            if (!value["exclude"].is_boolean()) {
                resgen::printError( PrintFileInfo {
                    .fileName = fileName,
                    .boldText = true,
                }, "expected bool for \"exclude\" of rule \"{}\"", pattern);
                return false;
            }

            if (value["exclude"].get<bool>())
                flags |= SymRule::FLAG_EXCLUDE;
        }

        if (value.contains("partial_match")) {
            if (!value["partial_match"].is_boolean()) {
                resgen::printError( PrintFileInfo {
                    .fileName = fileName,
                    .boldText = true,
                }, "expected bool for \"partial_match\" of rule \"{}\"", pattern);
                return false;
            }

            if (value["partial_match"].get<bool>())
                flags |= SymRule::FLAG_PARTIAL_MATCH;
        }

        // Add rule.
        try {
            rules.emplace_back(SymRule(pattern, std::move(name), flags));
            rulePatterns.insert(pattern);
        } catch (const InvalidRuleException& ex) {
            resgen::printError(PrintFileInfo {
                .fileName = fileName,
                .boldText = true,
            }, ex.what());
            return false;
        }
    }

    m_RulePatterns.merge(std::move(rulePatterns));

    m_Rules.reserve(m_Rules.size() + rules.size());
    std::move(rules.begin(), rules.end(), std::back_inserter(m_Rules));
    return true;
}

bool SymDefs::parseFile(const std::filesystem::path& path) {
    const auto fileName = path.filename().string();
    
    std::ifstream f(path);
    if (!f.is_open()) {
        resgen::printError(PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "could not open file");
        return false;
    }

    const std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return parseObject(fileName, content);
}

static std::size_t checkSym(std::string_view s) {
    for (auto i = 0u; i < s.size(); ++i) {
        if (s[i] <= '\x20' || s[i] >= '\x7F')
            return i;
    }

    return std::string_view::npos;
}

// Defined in Binary.cpp
extern bool parseBinary(const std::filesystem::path& path, SymList& out);

static bool parseList(const std::filesystem::path& path, SymList& out) {
    const auto fileName = path.filename().string();
    std::unordered_map<std::string, std::size_t> syms;
    std::string tmp;

    // Read list.
    std::ifstream f(path);
    if (!f.is_open()) {
        resgen::printError(PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "could not open file");
        return false;
    }

    std::size_t line = 1;
    while(std::getline(f, tmp)) {
        if (tmp.empty())
            continue;

        // Check characters.
        if (auto pos = checkSym(tmp); pos != std::string_view::npos) {
            resgen::printError(PrintFileInfo {
                .fileName = fileName,
                .line = line,
                .column = pos + 1,
                .boldText = true,
            }, "invalid character");
            return false;
        }

        // Check for duplicates.
        auto tmpIt = syms.find(tmp);
        auto otherIt = out.find(SymEntry{ tmp, false });

        if ((tmpIt != syms.end()) || (otherIt != out.end())) {
            resgen::printWarning(PrintFileInfo {
                .fileName = fileName,
                .line = line,
                .column = 1,
                .boldText = true,
            }, "duplicate symbol, ignored");
            resgen::printTokenMark(tmp, 0);

            if (otherIt != out.end())
                resgen::printNote("declared in a different file");
        } else {
            syms.insert({ std::move(tmp), line });
        }

        ++line;
    }

    auto it = syms.begin();
    while (it != syms.end()) {
        auto node = syms.extract(it);
        out.insert({ node.key(), false });
        it = syms.begin();
    }

    return true;
}

static bool isExecutable(const std::filesystem::path& path) {
    constexpr static const char* EXTENSIONS[] = {
        ".elf",
        ".so"
    };

    auto ext = path.extension().string();
    for (auto& c : ext)
        c = std::tolower(c);

    for (auto i = 0; i < (sizeof(EXTENSIONS)/sizeof(EXTENSIONS[0])); ++i) {
        if (ext == EXTENSIONS[i])
            return true;
    }

    return false;
}

bool resgen::parseSymInput(const std::filesystem::path& path, SymList& out) {
    return isExecutable(path) ? parseBinary(path, out) : parseList(path, out);
}