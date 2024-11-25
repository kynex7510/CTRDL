#include "SymParse.h"

#include <fstream>

using namespace resgen;

namespace {

class LineStream {
    std::string m_Line;
    std::size_t m_Index = 0;

public:
    LineStream(const std::string& line) { m_Line = line; }

    char getNextToken() {
        if (m_Index < m_Line.size())
            return m_Line[m_Index++];

        return 0;
    }
};

} // anonymous namespace

bool SymMapBuilder::parse(const std::filesystem::path& path) {
    m_LastPath = path;

    std::ifstream f(m_LastPath);
    if (!f.is_open()) {
        resgen::printError(PrintFileInfo {
            .fileName = lastFileName(),
            .boldText = true,
        }, "could not open file");
        return false;
    }

    std::string line;
    
    m_CurrentLine = 0;

    while (std::getline(f, line)) {
        // Skip empty lines.
        if (line.empty())
            continue;

        LineStream l(line);
        std::string currentImportName;
        std::string currentTargetName;
        bool feedTarget = false;
        std::size_t skippedTokens = 0;

        ++m_CurrentLine;
        m_CurrentColumn = 0;

        // Parse line.
        while (auto c = l.getNextToken()) {
            ++m_CurrentColumn;

            // Ignore whitespaces.
            if (c == ' ') {
                ++skippedTokens;
                continue;
            }

            // Ignore comments.
            if (c == '#') {
                ++skippedTokens;
                break;
            }

            // Set feed target.
            if (c == ':') {
                if (feedTarget || currentImportName.empty()) {
                    printError("invalid token, expected {}", feedTarget ? "line end" : "symbol name");
                    printTokenMark(line, m_CurrentColumn - skippedTokens - 1);
                    return false;
                }

                feedTarget = true;
                continue;
            }

            if (feedTarget) {
                currentTargetName.push_back(c);
            } else {
                currentImportName.push_back(c);
            }
        }

        if (feedTarget && currentTargetName.empty()) {
            printError("invalid token, expected symbol name");
            printTokenMark(line, m_CurrentColumn - skippedTokens); // Improper ending.
            return false;
        }

        if (!feedTarget)
            currentTargetName = currentImportName;

        // Warn about redeclaration.
        if (auto it = m_SymMap.find(currentImportName); it != m_SymMap.end()) {
            printWarning("\"{}\" redeclared, new target is \"{}\"", it->first, currentTargetName);
            printTokenMark(line, 0);
            printNote("previous target was \"{}\"", it->second);
        }

        m_SymMap[currentImportName] = currentTargetName;
    }

    return true;
}

bool resgen::parseSymList(const std::filesystem::path& path, SymList& out) {
    std::ifstream f(path);

    if (!f.is_open()) {
        resgen::printError(PrintFileInfo {
            .fileName = path.filename().string(),
            .boldText = true,
        }, "could not open file");
        return false;
    }

    out.clear();

    std::string line;
    while (std::getline(f, line)) {
        // Skip empty lines.
        if (line.empty())
            continue;

        out.insert(line);
    }

    return true;
}