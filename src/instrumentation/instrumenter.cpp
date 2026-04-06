#include "instrumenter.h"

#include <cstdlib>
#include <filesystem>
#include <godot_cpp/variant/utility_functions.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "rewriter.h"
extern "C" {
#include <tree_sitter/api.h>
const TSLanguage* tree_sitter_gdscript(void);
}

namespace godot {

static bool is_debug_enabled() {
    const char* v = std::getenv("NANO_COVERAGE_DEBUG");
    return v && *v && std::string_view(v) != "0";
}

static size_t find_line_start(const std::string& src, size_t byte_pos) {
    if (byte_pos > src.size())
        byte_pos = src.size();
    while (byte_pos > 0) {
        if (src[byte_pos - 1] == '\n')
            break;
        byte_pos--;
    }
    return byte_pos;
}

static size_t find_line_end(const std::string& src, size_t line_start) {
    size_t i = line_start;
    while (i < src.size() && src[i] != '\n')
        i++;
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

static bool is_continuation_line(const std::string& src, size_t line_start) {
    size_t i = line_start;
    while (i < src.size()) {
        const char c = src[i];
        if (c == ' ' || c == '\t') {
            i++;
            continue;
        }
        if (c == '.') {
            return true;
        }
        return false;
    }
    return false;
}

static bool is_line_already_instrumented(const std::string& src, size_t line_start) {
    const size_t line_end = find_line_end(src, line_start);
    const std::string_view line(src.data() + line_start, line_end - line_start);
    return line.find("NanoCoverage.hit(") != std::string_view::npos;
}

static bool is_comment_node(TSNode n) {
    const char* t = ts_node_type(n);
    return t && std::string_view(t).find("comment") != std::string_view::npos;
}

static bool is_function_def_node_type(std::string_view t) {
    return t.find("function") != std::string_view::npos || t.find("method") != std::string_view::npos ||
           t.find("func") != std::string_view::npos;
}

static bool is_block_like_node_type(std::string_view t) {
    if (t == "match_body") {
        return false;
    }
    return t == "block" || t.find("block") != std::string_view::npos || t.find("body") != std::string_view::npos ||
           t.find("suite") != std::string_view::npos;
}

static bool has_function_ancestor(TSNode n) {
    for (TSNode cur = ts_node_parent(n); !ts_node_is_null(cur); cur = ts_node_parent(cur)) {
        if (is_function_def_node_type(ts_node_type(cur)))
            return true;
    }
    return false;
}

static std::string escape_gd_string(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\\')
            out += "\\\\";
        else if (c == '"')
            out += "\\\"";
        else if (c == '\n')
            out += "\\n";
        else if (c == '\r')
            out += "\\r";
        else if (c == '\t')
            out += "\\t";
        else
            out.push_back(c);
    }
    return out;
}

static std::string make_injected_line(const std::string& indent, const std::string& file_lit, uint32_t line_1_based) {
    return indent + "NanoCoverage.hit(\"" + file_lit + "\", " + std::to_string(line_1_based) + ")\n";
}

static void collect_insertions(TSNode node, const std::string& src, const std::string& file_lit,
                               std::vector<TextInsertion>& out_insertions, std::vector<uint32_t>& out_lines) {
    const std::string_view type = ts_node_type(node);

    // 1. Standard Block Instrumentation
    if (is_block_like_node_type(type) && has_function_ancestor(node)) {
        const uint32_t n = ts_node_named_child_count(node);
        for (uint32_t i = 0; i < n; i++) {
            const TSNode child = ts_node_named_child(node, i);
            if (ts_node_is_null(child) || is_comment_node(child))
                continue;

            const std::string_view child_type = ts_node_type(child);
            
            // Never inject above structural flow clauses directly in the generic loop.
            if (child_type == "elif_clause" || child_type == "else_clause" || 
                child_type == "match_branch" || child_type == "pattern_section") {
                continue; 
            }

            const size_t stmt_start = (size_t)ts_node_start_byte(child);
            const size_t line_start = find_line_start(src, stmt_start);

            if (is_line_already_instrumented(src, line_start))
                continue;

            if (is_continuation_line(src, line_start)) {
                continue;
            }

            const TSPoint pt = ts_node_start_point(child);
            const uint32_t line_1_based = pt.row + 1;

            size_t first_non_ws = line_start;
            while (first_non_ws < src.size() && (src[first_non_ws] == ' ' || src[first_non_ws] == '\t')) {
                first_non_ws++;
            }

            bool is_inline = stmt_start > first_non_ws;
            out_lines.push_back(line_1_based);

            size_t current_idx = out_insertions.size();

            if (is_inline) {
                std::string injected = "NanoCoverage.hit(\"" + file_lit + "\", " + std::to_string(line_1_based) + "); ";
                out_insertions.push_back(TextInsertion{stmt_start, injected, current_idx});
            } else {
                const std::string indent = get_line_indent(src, line_start);
                out_insertions.push_back(TextInsertion{line_start, make_injected_line(indent, file_lit, line_1_based), current_idx});
            }
        }
    }

    // 2. Structural Branch Logic (Elif / Else / Match Branch)
    if ((type == "elif_clause" || type == "else_clause" || type == "match_branch") && has_function_ancestor(node)) {
        TSNode body = {};
        for (uint32_t i = 0; i < ts_node_child_count(node); i++) {
            TSNode c = ts_node_child(node, i);
            if (is_block_like_node_type(ts_node_type(c))) {
                body = c;
                break;
            }
        }

        if (!ts_node_is_null(body) && ts_node_named_child_count(body) > 0) {
            const TSPoint branch_pt = ts_node_start_point(node);
            const uint32_t branch_line = branch_pt.row + 1;
            
            TSNode first_stmt = ts_node_named_child(body, 0);
            
            if (!ts_node_is_null(first_stmt) && !is_comment_node(first_stmt)) {
                const size_t stmt_start = (size_t)ts_node_start_byte(first_stmt);
                const size_t line_start = find_line_start(src, stmt_start);
                
                out_lines.push_back(branch_line);

                size_t first_non_ws = line_start;
                while (first_non_ws < src.size() && (src[first_non_ws] == ' ' || src[first_non_ws] == '\t')) {
                    first_non_ws++;
                }

                bool is_inline = stmt_start > first_non_ws;
                size_t current_idx = out_insertions.size();

                if (is_inline) {
                    std::string injected = "NanoCoverage.hit(\"" + file_lit + "\", " + std::to_string(branch_line) + "); ";
                    out_insertions.push_back(TextInsertion{stmt_start, injected, current_idx});
                } else {
                    const std::string indent = get_line_indent(src, line_start);
                    out_insertions.push_back(TextInsertion{line_start, make_injected_line(indent, file_lit, branch_line), current_idx});
                }
            }
        }
    }

    // 3. Recursive traversal
    const uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        const TSNode c = ts_node_child(node, i);
        if (!ts_node_is_null(c)) {
            collect_insertions(c, src, file_lit, out_insertions, out_lines);
        }
    }
}

InstrumentResult Instrumenter::instrument_text(const std::string& utf8_code, const std::string& res_path) {
    InstrumentResult result;
    result.ok = false;
    result.instrumented_code = utf8_code;

    TSParser* parser = ts_parser_new();
    if (!parser) {
        result.error_message = "Failed to create tree-sitter parser";
        return result;
    }

    if (!ts_parser_set_language(parser, tree_sitter_gdscript())) {
        result.error_message = "tree-sitter-gdscript language init failed";
        ts_parser_delete(parser);
        return result;
    }

    TSTree* tree = ts_parser_parse_string(parser, nullptr, utf8_code.data(), (uint32_t)utf8_code.size());
    if (!tree) {
        result.error_message = "Failed to parse code";
        ts_parser_delete(parser);
        return result;
    }

    const TSNode root = ts_tree_root_node(tree);

    std::vector<TextInsertion> insertions;
    insertions.reserve(64);

    const std::string file_lit = escape_gd_string(res_path);
    collect_insertions(root, utf8_code, file_lit, insertions, result.covered_lines);
    result.insertions = (int)insertions.size();

    if (insertions.empty()) {
        ts_tree_delete(tree);
        ts_parser_delete(parser);
        result.ok = true;
        return result;
    }

    result.instrumented_code = Rewriter::apply(utf8_code, std::move(insertions));
    ts_tree_delete(tree);
    ts_parser_delete(parser);

    result.ok = true;
    return result;
}

}  // namespace godot