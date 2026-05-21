#pragma once

#include "core/panic.h"
#include "core/types.h"
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace z {

struct Span {
    u32 index;
    u16 len;

    Span() = default;
    Span(u32 index, u16 len) : index(index), len(len) {};

    Span& operator+=(const Span& rhs) {
        auto start = index < rhs.index ? index : rhs.index;

        auto lhs_end = index + len;
        auto rhs_end = rhs.index + rhs.len;
        auto end = lhs_end > rhs_end ? lhs_end : rhs_end;

        index = start;
        len = end - index;

        return *this;
    }

    friend Span operator+(Span lhs, const Span& rhs) {
        auto index = lhs.index < rhs.index ? lhs.index : rhs.index;

        auto lhs_end = lhs.index + lhs.len;
        auto rhs_end = rhs.index + rhs.len;
        auto end = lhs_end > rhs_end ? lhs_end : rhs_end;

        return Span(index, end - index);
    }

    friend bool operator<(const Span& l, const Span& r) {
        return l.index < r.index;
    }
};

class LinePos {
    usize line;
    usize col;

public:
    LinePos(usize line, usize col) : line(line), col(col) {};

    [[nodiscard]] usize get_line() const { return line; }
    [[nodiscard]] usize get_col() const { return col; }
};

class SourceManager {
    std::vector<char> input;
    std::optional<std::filesystem::path> path;
    std::vector<std::pair<size_t, size_t>> lines;

    SourceManager(std::filesystem::path& path, std::vector<char> input)
        : input(std::move(input)), path(path) {
        calculate_line_indices();
    };
    explicit SourceManager(const std::string& input)
        : input(input.begin(), input.end()), path(std::nullopt) {
        calculate_line_indices();
    };

    void calculate_line_indices() {
        lines.clear();
        usize start = 0;
        for (size_t i = 0; i < input.size(); i++) {
            if (input[i] == '\n') {
                lines.emplace_back(start, i);
                start = i + 1;
            }
        }
        lines.emplace_back(start, input.size() - 1);
    }

public:
    static std::unique_ptr<SourceManager> Create(const std::string& input) {
        return std::unique_ptr<SourceManager>(new SourceManager(input));
    }

    static std::unique_ptr<SourceManager>
    CreateFromPath(const std::string& path_) {
        auto path = std::filesystem::path(path_);
        if (!std::filesystem::exists(path)) {
            std::cerr << std::format("error: no such file or directory: '{}'\n",
                                     path_);
            return nullptr;
        }

        auto file = std::ifstream(path, std::ios::ate);
        if (!file.is_open()) {
            std::cerr << std::format("error: could not open file: '{}'\n",
                                     path_);
            return nullptr;
        }

        auto size = file.tellg();
        file.seekg(0);
        auto input = std::vector<char>(size);
        if (!file.read(input.data(), size)) {
            std::cerr << std::format("error: could not read file: '{}'\n",
                                     path_);
            return nullptr;
        }

        return std::unique_ptr<SourceManager>(new SourceManager(path, input));
    }

    [[nodiscard]] std::optional<char> get_char(const usize index) {
        if (!in_bounds(index)) {
            return std::nullopt;
        }

        return input[index];
    }

    [[nodiscard]] std::string get_line(const usize line) const {
        const auto [start, end] = lines[line - 1];
        return std::string(&input[start], end - start);
    }

    [[nodiscard]] bool in_bounds(const usize index) const {
        return index < input.size();
    }

    [[nodiscard]] const char* get_char_ptr(const usize index) const {
        if (!in_bounds(index))
            return nullptr;

        return &input[index];
    }

    [[nodiscard]] std::string get_path() const {
        if (path) {
            return path->string();
        }

        return std::string("<stdin>");
    }

    [[nodiscard]] std::string_view get_string(const Span& span) const {
        return std::string_view(get_char_ptr(span.index), span.len);
    }

    [[nodiscard]] LinePos get_pos(const Span& span, bool& multiline) const {
        for (usize i = 0; i < lines.size(); i++) {
            const auto& [start, end] = lines[i];

            if (span.index >= start && span.index <= end) {
                multiline = span.index + span.len > end;
                return LinePos(i + 1, span.index - start);
            }
        }

        panic("Invalid span");
    }

    [[nodiscard]] LinePos get_last_line(const Span& span,
                                        const LinePos& first) {
        const auto end_idx = span.index + span.len - 1;
        for (usize i = first.get_line() - 1; i < lines.size(); i++) {
            if (end_idx <= lines[i].second) {
                return LinePos(i + 1, end_idx - lines[i].first);
            }
        }

        panic("Invalid span");
    }
};
} // namespace z
