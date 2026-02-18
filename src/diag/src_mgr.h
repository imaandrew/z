#pragma once

#include "core/panic.h"
#include <cstddef>
#include <cstdint>
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
    std::uint32_t index;
    std::uint16_t len;

    Span() = default;
    Span(std::uint32_t index, std::uint16_t len) : index(index), len(len) {};

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
    std::size_t line;
    std::size_t col;

public:
    LinePos(std::size_t line, std::size_t col) : line(line), col(col) {};

    [[nodiscard]] std::size_t get_line() const { return line; }
    [[nodiscard]] std::size_t get_col() const { return col; }
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
        std::size_t start = 0;
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

    [[nodiscard]] std::optional<char> get_char(const std::size_t index) {
        if (!in_bounds(index)) {
            return std::nullopt;
        }

        return input[index];
    }

    [[nodiscard]] std::string get_line(const std::size_t line) const {
        const auto [start, end] = lines[line - 1];
        return std::string(&input[start], end - start);
    }

    [[nodiscard]] bool in_bounds(const std::size_t index) const {
        return index < input.size();
    }

    [[nodiscard]] const char* get_char_ptr(const std::size_t index) const {
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
        for (std::size_t i = 0; i < lines.size(); i++) {
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
        for (std::size_t i = first.get_line() - 1; i < lines.size(); i++) {
            if (end_idx <= lines[i].second) {
                return LinePos(i + 1, end_idx - lines[i].first);
            }
        }

        panic("Invalid span");
    }
};
} // namespace z
