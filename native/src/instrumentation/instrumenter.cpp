#include "instrumenter.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <godot_cpp/variant/utility_functions.hpp>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "rewriter.h"

extern "C" {
#include <tree_sitter/api.h>
const TSLanguage* tree_sitter_gdscript(void);
}

namespace godot {

/// Optional diagnostics to understand parser/grammar differences in dev setups.
static bool is_debug_enabled() {
    const char* v = std::getenv("NANO_COVERAGE_DEBUG");
    return v && *v && std::string_view(v) != "0";
}

/// File I/O is kept centralized so the AST/injection logic stays easy to follow.
static bool read_all_bytes(const std::filesystem::path& p, std::string& out) {
    std::ifstream f(p, std::ios::binary);
    if (!f.is_open()) {
        return false;
    }

    f.seekg(0, std::ios::end);
    const size_t sz = (size_t)f.tellg();
    f.seekg(0, std::ios::beg);

    out.resize(sz);
    if (sz > 0) {
        f.read(out.data(), (std::streamsize)sz);
    }
    return true;
}

static bool write_all_bytes(const std::filesystem::path& p, const std::string& data) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f.is_open()) {
        return false;
    }
    f.write(data.data(), (std::streamsize)data.size());
    return true;
}

static size_t find_line_start(const std::string& src, size_t byte_pos) {
    if (byte_pos > src.size()) {
        byte_pos = src.size();
    }
    while (byte_pos > 0) {
        if (src[byte_pos - 1] == '\n') {
            break;
        }
        byte_pos--;
    }
    return byte_pos;
}

static size_t find_line_end(const std::string& src, size_t line_start) {
    size_t i = line_start;
    while (i < src.size() && src[i] != '\n') {
        i++;
    }
    return i;
}

static std::string get_line_indent(const std::string& src, size_t line_start) {
    size_t i = line_start;
    while (i < src.size()) {
        const char c = src[i];
        if (c == ' ' || c == '\t') {
            i++;
            continue;
        }
        break;
    }
    return src.substr(line_start, i - line_start);
}

/// Avoid duplicate insertions if the instrumenter runs multiple times on the same file.
static bool is_line_already_instrumented(const std::string& src, size_t line_start) {
    const size_t line_end = find_line_end(src, line_start);
    const std::string_view line(src.data() + line_start, line_end - line_start);
    return line.find("NanoCoverage.hit(") != std::string_view::npos;
}

static bool is_comment_node(TSNode n) {
    const char* t = ts_node_type(n);
    if (!t) {
        return false;
    }
    return std::string_view(t).find("comment") != std::string_view::npos;
}

/// Matching is permissive to tolerate minor tree-sitter-gdscript grammar changes.
static bool is_function_def_node_type(std::string_view t) {
    return t.find("function") != std::string_view::npos || t.find("method") != std::string_view::npos ||
           t.find("func") != std::string_view::npos;
}

static bool is_block_like_node_type(std::string_view t) {
    return t == "block" || t.find("block") != std::string_view::npos || t.find("body") != std::string_view::npos ||
           t.find("suite") != std::string_view::npos;
}

static bool has_function_ancestor(TSNode n) {
    for (TSNode cur = ts_node_parent(n); !ts_node_is_null(cur); cur = ts_node_parent(cur)) {
        if (is_function_def_node_type(ts_node_type(cur))) {
            return true;
        }
    }
    return false;
}

static void dump_node_types(TSNode root, const std::string& res_path) {
    std::unordered_set<std::string> types;
    std::vector<TSNode> stack;
    stack.push_back(root);

    while (!stack.empty()) {
        const TSNode n = stack.back();
        stack.pop_back();

        if (const char* t = ts_node_type(n)) {
            types.insert(std::string(t));
        }

        const uint32_t c = ts_node_child_count(n);
        for (uint32_t i = 0; i < c; i++) {
            const TSNode ch = ts_node_child(n, i);
            if (!ts_node_is_null(ch)) {
                stack.push_back(ch);
            }
        }
    }

    UtilityFunctions::print("NanoCoverage: AST node types for ", String(res_path.c_str()), " (", (int)types.size(),
                            " types; first ~60 shown)");

    int shown = 0;
    for (const auto& s : types) {
        UtilityFunctions::print("  - ", String(s.c_str()));
        if (++shown >= 60) {
            UtilityFunctions::print("  ... (truncated)");
            break;
        }
    }
}

static std::string escape_gd_string(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 8);

    for (char c : s) {
        switch (c) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out.push_back(c);
                break;
        }
    }
    return out;
}

/// Builds the injected line.
/// Assumes the project guarantees the global `NanoCoverage` is available (autoload).
/// If memory becomes a concern in very large projects, consider compacting file identifiers at runtime.
static std::string make_injected_line(const std::string& indent, const std::string& file_lit, uint32_t line_1_based) {
    return indent + "NanoCoverage.hit(\"" + file_lit + "\", " + std::to_string(line_1_based) + ")\n";
}

static void collect_insertions(TSNode node, const std::string& src, const std::string& file_lit,
                               std::vector<TextInsertion>& out) {
    const std::string_view type = ts_node_type(node);

    if (is_block_like_node_type(type) && has_function_ancestor(node)) {
        const uint32_t n = ts_node_named_child_count(node);
        for (uint32_t i = 0; i < n; i++) {
            const TSNode child = ts_node_named_child(node, i);
            if (ts_node_is_null(child) || is_comment_node(child)) {
                continue;
            }

            const size_t stmt_start = (size_t)ts_node_start_byte(child);
            const size_t line_start = find_line_start(src, stmt_start);

            if (is_line_already_instrumented(src, line_start)) {
                continue;
            }

            const TSPoint pt = ts_node_start_point(child);
            const uint32_t line_1_based = pt.row + 1;

            const std::string indent = get_line_indent(src, line_start);
            out.push_back(TextInsertion{line_start, make_injected_line(indent, file_lit, line_1_based)});
        }
    }

    const uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        const TSNode c = ts_node_child(node, i);
        if (!ts_node_is_null(c)) {
            collect_insertions(c, src, file_lit, out);
        }
    }
}

bool Instrumenter::instrument_file_in_place(const std::filesystem::path& abs_path, const std::string& res_path,
                                            int* out_insertions) {
    if (out_insertions) {
        *out_insertions = 0;
    }

    std::string src;
    if (!read_all_bytes(abs_path, src)) {
        UtilityFunctions::printerr("NanoCoverage: failed to read: ", String(abs_path.string().c_str()));
        return false;
    }

    TSParser* parser = ts_parser_new();
    if (!parser) {
        return false;
    }

    if (!ts_parser_set_language(parser, tree_sitter_gdscript())) {
        UtilityFunctions::printerr("NanoCoverage: tree-sitter-gdscript language init failed");
        ts_parser_delete(parser);
        return false;
    }

    TSTree* tree = ts_parser_parse_string(parser, nullptr, src.data(), (uint32_t)src.size());
    if (!tree) {
        ts_parser_delete(parser);
        return false;
    }

    const TSNode root = ts_tree_root_node(tree);

    std::vector<TextInsertion> insertions;
    insertions.reserve(64);

    const std::string file_lit = escape_gd_string(res_path);
    collect_insertions(root, src, file_lit, insertions);

    if (out_insertions) {
        *out_insertions = (int)insertions.size();
    }

    if (insertions.empty()) {
        if (is_debug_enabled()) {
            UtilityFunctions::print("NanoCoverage: No insertions for ", String(res_path.c_str()),
                                    " (likely node-type mismatch; dumping node types)");
            dump_node_types(root, res_path);
        }
        ts_tree_delete(tree);
        ts_parser_delete(parser);
        return true;
    }

    std::string out_text = Rewriter::apply(src, std::move(insertions));

    ts_tree_delete(tree);
    ts_parser_delete(parser);

    if (!write_all_bytes(abs_path, out_text)) {
        UtilityFunctions::printerr("NanoCoverage: failed to write: ", String(abs_path.string().c_str()));
        return false;
    }

    return true;
}

}  // namespace godot
