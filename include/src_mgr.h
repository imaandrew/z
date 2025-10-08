#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
    std::vector<size_t> line_indices;

    SourceManager(std::filesystem::path& path, std::vector<char> input)
        : input(std::move(input)), path(path) {};
    explicit SourceManager(std::vector<char> input)
        : input(std::move(input)), path(std::nullopt) {};

public:
    static std::optional<SourceManager> Create(std::vector<char> input) {
        return SourceManager(std::move(input));
    }

    static std::optional<SourceManager> Create(const char* path_) {
        auto path = std::filesystem::path(path_);
        if (!std::filesystem::exists(path)) {
            std::cerr << std::format("error: no such file or directory: '{}'\n", path_);
            return std::nullopt;
        }

        auto file = std::ifstream(path, std::ios::ate);
        if (!file.is_open()) {
            std::cerr << std::format("error: could not open file: '{}'\n", path_);
            return std::nullopt;
        }

        auto size = file.tellg();
        file.seekg(0);
        auto input = std::vector<char>(size);
        if (!file.read(input.data(), size)) {
            std::cerr << std::format("error: could not read file: '{}'\n", path_);
            return std::nullopt;
        }

        return SourceManager(path, input);
    }

    [[nodiscard]] std::optional<char> get_char(const std::size_t index) {
        if (in_bounds(index)) {
            return std::nullopt;
        }

        if (input[index] == '\n' && (line_indices.empty() || line_indices.back() != index))
            line_indices.push_back(index);

        return input[index];
    }

    [[nodiscard]] std::optional<std::string>
    get_line(const std::size_t line) const {
        const std::size_t start = line_indices[line - 1] + 1;
        const std::size_t end = line_indices[line] - 1;
        return std::string(&input[start], end - start);
    }

    [[nodiscard]] bool in_bounds(const std::size_t index) const {
        return index >= input.size();
    }

    [[nodiscard]] const char* get_char_ptr(const std::size_t index) const {
        if (in_bounds(index))
            return nullptr;

        return &input[index];
    }

    [[nodiscard]] std::string get_path() const {
        if (path) {
            return path->string();
        }

        return std::string("asdf");
    }

    [[nodiscard]] std::string_view get_string(const Span& span) const {
        return std::string_view(get_char_ptr(span.index), span.len);
    }

    [[nodiscard]] LinePos get_pos(const Span& span) const {
        auto line = std::ranges::lower_bound(line_indices, span.index) - line_indices.begin();
        auto line_start = (line == 0) ? 0 : line_indices[line - 1] + 1;
        
        return LinePos(line + 1, span.index - line_start + 1);
    }
};
