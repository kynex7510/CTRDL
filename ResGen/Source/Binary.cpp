#include <elf.h>

#include "Symbol.h"
#include "Print.h"

#include <algorithm>
#include <fstream>
#include <vector>
#include <span>

using namespace resgen;

namespace {

class BinaryReader {
    std::ifstream& m_Stream;

public:
    BinaryReader(std::ifstream& stream) : m_Stream(stream) {}

    bool readMulti(std::size_t offset, std::span<std::uint8_t> buffer) {
        m_Stream.seekg(offset, m_Stream.beg);
        m_Stream.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        return true;
    }

    template <typename T>
    bool readMulti(std::size_t offset, std::vector<T>& buffer) {
        return readMulti(offset, std::span<std::uint8_t> { reinterpret_cast<std::uint8_t*>(buffer.data()), sizeof(T) * buffer.size() });
    }

    template <typename T>
    bool readSingle(std::size_t offset, T& out) { return readMulti(offset, std::span<std::uint8_t>{ reinterpret_cast<std::uint8_t*>(&out), sizeof(T) }); }
};

} // anonymous namespace

static bool is64Bit(std::istream& f) {
    f.seekg(EI_CLASS, f.beg);
    return f.get() == ELFCLASS64;
}

#define LOG(s) resgen::print({}, {}, s)

// TODO: the behaviour of this code is probably incorrent, as we should find import symbols from relocations.
template <typename WordType, typename HeaderType, typename SegmentType, typename DynEntryType, typename SymEntryType>
static bool parse(const std::string& fileName, std::ifstream& f, SymList& out) {
    BinaryReader reader(f);

    // Read header.
    HeaderType header;
    if (!reader.readSingle(0, header)) {
        resgen::printError(resgen::PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "could not read ELF header");
        return false;
    }

    if (!std::equal(header.e_ident, header.e_ident + SELFMAG, ELFMAG, ELFMAG + SELFMAG)) {
        resgen::printError(resgen::PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "ELF magic mismatch");
        return false;
    }

    // Read segments.
    std::vector<SegmentType> segments;
    segments.resize(header.e_phnum);

    if (!reader.readMulti(header.e_phoff, segments)) {
        resgen::printError(resgen::PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "could not read ELF segments");
        return false;
    }

    // Read dynamic segment.
    auto dyn = segments.begin();
    while (dyn != segments.end()) {
        if (dyn->p_type == PT_DYNAMIC)
            break;

        ++dyn;
    }

    if (dyn == segments.end())
        return true; // No segment = no symbols.

    std::vector<DynEntryType> dynEntries;
    dynEntries.resize(dyn->p_filesz / sizeof(DynEntryType));

    if (!reader.readMulti(dyn->p_offset, dynEntries)) {
        resgen::printError(resgen::PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "could not read ELF dynamic entries");
        return false;
    }

    // Read symbol table.
    auto hashTab = dynEntries.begin();
    while (hashTab != dynEntries.end()) {
        if (hashTab->d_tag == DT_HASH)
            break;

        ++hashTab;
    }

    if (hashTab == dynEntries.end())
        return true; // No hash table = no symbols.

    WordType numSyms = 0;
    if (!reader.readSingle(hashTab->d_un.d_ptr + sizeof(WordType), numSyms)) {
        resgen::printError(resgen::PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "could not read number of ELF symbols from hash dynamic entry");
        return false;
    }

    auto symTab = dynEntries.begin();
    while (symTab != dynEntries.end()) {
        if (symTab->d_tag == DT_SYMTAB)
            break;

        ++symTab;
    }

    if (symTab == dynEntries.end()) {
        resgen::printError(resgen::PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "could not find ELF symbol table entry");
        return false;
    }

    std::vector<SymEntryType> symEntries;
    symEntries.resize(numSyms);

    if (!reader.readMulti(symTab->d_un.d_ptr, symEntries)) {
        resgen::printError(resgen::PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "could not read ELF symbol table");
        return false;
    }

    // Read string table.
    auto strTab = dynEntries.begin();
    while (strTab != dynEntries.end()) {
        if (strTab->d_tag == DT_STRTAB)
            break;

        ++strTab;
    }

    if (strTab == dynEntries.end()) {
        resgen::printError(resgen::PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "could not find ELF string table entry");
        return false;
    }

    auto strSz = dynEntries.begin();
    while (strSz != dynEntries.end()) {
        if (strSz->d_tag == DT_STRSZ)
            break;

        ++strSz;
    }

    if (strSz == dynEntries.end()) {
        resgen::printError(resgen::PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "could not find ELF string size entry");
        return false;
    }

    std::vector<char> stringTable;
    stringTable.resize(strSz->d_un.d_val);

    if (!reader.readMulti(strTab->d_un.d_ptr, stringTable)) {
        resgen::printError(resgen::PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "could not read ELF string table");
    }

    // Read symbols.
    const auto& isWeak = [](unsigned char info) {
        if constexpr (std::is_same_v<HeaderType, Elf64_Ehdr>) {
            return ELF64_ST_BIND(info) == STB_WEAK;
        } else {
            return ELF32_ST_BIND(info) == STB_WEAK;
        }
    };
    
    for (auto& sym : symEntries) {
        if (sym.st_name == STN_UNDEF || sym.st_shndx != SHN_UNDEF)
            continue;

        const char* name = &stringTable[sym.st_name];
        out.insert({name, isWeak(sym.st_info) });
    }

    return true;
}

bool parseBinary(const std::filesystem::path& path, SymList& out) {
    const auto fileName = path.filename().string();

    // Open file.
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        resgen::printError(resgen::PrintFileInfo {
            .fileName = fileName,
            .boldText = true,
        }, "could not open file");
        return false;
    }

    return is64Bit(f)
        ? parse<Elf64_Word, Elf64_Ehdr, Elf64_Phdr, Elf64_Dyn, Elf64_Sym>(fileName, f, out)
        : parse<Elf32_Word, Elf32_Ehdr, Elf32_Phdr, Elf32_Dyn, Elf32_Sym>(fileName, f, out);
}