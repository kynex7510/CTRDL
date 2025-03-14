#ifndef _RESGEN_PRINT_H
#define _RESGEN_PRINT_H

#include <fmt/printf.h>
#include <fmt/color.h>

#include <string_view>
#include <optional>

namespace resgen {

struct PrintFileInfo {
    std::string_view fileName;
    std::size_t line = 0;
    std::size_t column = 0;
    bool boldText = false;
};

struct PrintCategoryInfo {
    std::string_view prefix;
    fmt::color prefixColor;
    bool boldText = false;
};

template <typename ... Args>
void print(std::optional<PrintFileInfo> fileInfo, std::optional<PrintCategoryInfo> catInfo, std::string_view fmt, Args&&... args) {
    if (fileInfo) {
        if (fileInfo->boldText) {
            if (!fileInfo->line || !fileInfo->column) {
                fmt::print(fmt::emphasis::bold, "{}: ", fileInfo->fileName);
            } else {
                fmt::print(fmt::emphasis::bold, "{}:{}:{}: ", fileInfo->fileName, fileInfo->line, fileInfo->column);
            }
        } else {
            if (!fileInfo->line || !fileInfo->column) {
                fmt::print("{}: ", fileInfo->fileName);
            } else {
                fmt::print("{}:{}:{}: ", fileInfo->fileName, fileInfo->line, fileInfo->column);
            }
        }
    }

    if (catInfo) {
        fmt::print(fmt::fg(catInfo->prefixColor) | fmt::emphasis::bold, "{}: ", catInfo->prefix);

        if (catInfo->boldText) {
            fmt::print(fmt::emphasis::bold, fmt::runtime(fmt), std::forward<Args>(args)...);
        } else {
            fmt::print(fmt::runtime(fmt), std::forward<Args>(args)...);
        }

        fmt::println("");
    } else {
        fmt::println(fmt::runtime(fmt), std::forward<Args>(args)...);
    }
}

template <typename ... Args>
void printNote(std::string_view fmt, Args&&... args) {
    print({}, PrintCategoryInfo {
            .prefix = "note",
            .prefixColor = fmt::color::magenta,
        }, fmt, std::forward<Args>(args)...);
}

template <typename ... Args>
void printWarning(std::optional<PrintFileInfo> fileInfo, std::string_view fmt, Args&&... args) {
    print(fileInfo, PrintCategoryInfo {
            .prefix = "warning",
            .prefixColor = fmt::color::yellow,
            .boldText = true,
        }, fmt, std::forward<Args>(args)...);
}

template <typename ... Args>
void printError(std::optional<PrintFileInfo> fileInfo, std::string_view fmt, Args&&... args) {
    print(fileInfo, PrintCategoryInfo {
            .prefix = "error",
            .prefixColor = fmt::color::crimson,
            .boldText = true,
        }, fmt, std::forward<Args>(args)...);
}

inline void printTokenMark(std::string_view line, std::size_t index) {
    fmt::println("\t{}", line);
    fmt::print(fmt::fg(fmt::color::forest_green) | fmt::emphasis::bold, "\t{}^\n", std::string(index, ' '));
}

} // namespace resgen

#endif /* _RESGEN_PRINT_H */