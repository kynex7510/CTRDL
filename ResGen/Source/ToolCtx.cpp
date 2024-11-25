#include "ToolCtx.h"

using namespace resgen;

bool ToolCtx::init(const CmdArgs& args) {
    SymMapBuilder symMapBuilder;

    for (const auto& definitionList : args.definitionLists()) {
        if (!symMapBuilder.parse(definitionList))
            return false;
    }

    m_SymsDefs = std::move(symMapBuilder.extractSymMap());

    for (const auto& excludedList : args.exclusionLists()) {
        if (!symMapBuilder.parse(excludedList))
            return false;
    }

    m_ExcludedSyms = symMapBuilder.extractSymList();
    m_ResolverName = args.resolverName();
    return true;
}