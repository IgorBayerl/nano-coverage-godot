#include "instrumenter.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <godot_cpp/variant/utility_functions.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "rewriter.h"
#include "source_reader.h"

extern "C" {
#include <tree_sitter/api.h>
const TSLanguage* tree_sitter_gdscript(void);
}

namespace godot {

static bool is_debug_enabled() {
    const char* v = std::getenv("NANO_COVERAGE_DEBUG");
    return v && *v && std::string_view(v) != "0";
}

static bool write_all_bytes(const std::filesystem::path& p, const std::string& data) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f.is_open())
        return false;
    f.write(data.data(), (std::streamsize)data.size());
    return true;
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
    if (is_block_like_node_type(type) && has_function_ancestor(node)) {
        const uint32_t n = ts_node_named_child_count(node);
        for (uint32_t i = 0; i < n; i++) {
            const TSNode child = ts_node_named_child(node, i);
            if (ts_node_is_null(child) || is_comment_node(child))
                continue;

            const size_t stmt_start = (size_t)ts_node_start_byte(child);
            const size_t line_start = find_line_start(src, stmt_start);

            if (is_line_already_instrumented(src, line_start))
                continue;

            const TSPoint pt = ts_node_start_point(child);
            const uint32_t line_1_based = pt.row + 1;

            const std::string indent = get_line_indent(src, line_start);

            // Record the coverable line
            out_lines.push_back(line_1_based);

            // Record the injection
            out_insertions.push_back(TextInsertion{line_start, make_injected_line(indent, file_lit, line_1_based)});
        }
    }

    const uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        const TSNode c = ts_node_child(node, i);
        if (!ts_node_is_null(c)) {
            collect_insertions(c, src, file_lit, out_insertions, out_lines);
        }
    }
}

// [Pure Pipeline]
// Takes raw source code (UTF-8), parses it with Tree-sitter, identifies coverable lines,
// and returns the instrumented code and metadata.
// This function performs NO file I/O and is side-effect free, making it ideal for unit testing.
InstrumentResult Instrumenter::instrument_text(const std::string& utf8_code, const std::string& res_path) {
    InstrumentResult result;
    result.ok = false;
    
    // Copy input code as it might be unmodified
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

    // Pass the vector to collect lines
    collect_insertions(root, utf8_code, file_lit, insertions, result.covered_lines);
    result.insertions = (int)insertions.size();

    // If no insertions, we are done
    if (insertions.empty()) {
        ts_tree_delete(tree);
        ts_parser_delete(parser);
        result.ok = true;
        return result;
    }

    // Rewrite code
    result.instrumented_code = Rewriter::apply(utf8_code, std::move(insertions));

    ts_tree_delete(tree);
    ts_parser_delete(parser);
    
    result.ok = true;
    return result;
}


// [I/O Wrapper]
// Handles the "dirty work" of interacting with the filesystem.
// * Reads the file using SourceReader (handling BOMs/encoding).
// * Delegates the logic to instrument_text().
// * Writes the result back to disk if changes were made.
bool Instrumenter::instrument_file(const String& path, const String& res_path, std::vector<uint32_t>* out_lines, int* out_insertions) {
    if (out_lines) out_lines->clear();
    if (out_insertions) *out_insertions = 0;

    // Read
    // We use NanoCoverage::SourceReader which we just imported
    NanoCoverage::ReadTextResult read_res = NanoCoverage::SourceReader::read_text_file(path.utf8().get_data());
    
    if (!read_res.ok) {
        UtilityFunctions::printerr("NanoCoverage: failed to read: ", path);
        // If needed, log read_res.error_message
        return false;
    }

    // Instrument
    std::string res_path_std = res_path.utf8().get_data();
    InstrumentResult inst_res = instrument_text(read_res.content, res_path_std);

    if (!inst_res.ok) {
        UtilityFunctions::printerr("NanoCoverage: instrumentation failed for: ", path, " error: ", String(inst_res.error_message.c_str()));
        return false;
    }

    // Output metadata
    if (out_lines) {
        *out_lines = inst_res.covered_lines;
    }
    if (out_insertions) {
        *out_insertions = inst_res.insertions;
    }

    // Write
    if (inst_res.insertions == 0 && read_res.content == inst_res.instrumented_code) {
        return true;
    }
    
    // We also typically return true if 0 insertions, same as before, but now we filled the metadata.
    if (inst_res.insertions == 0) {
        return true;
    }

    // Write back
    std::filesystem::path fs_path(path.utf8().get_data());
    if (!write_all_bytes(fs_path, inst_res.instrumented_code)) {
        UtilityFunctions::printerr("NanoCoverage: failed to write: ", path);
        return false;
    }

    return true;
}

}  // namespace godot