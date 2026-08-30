#include "shell_completion.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>
#include <unistd.h>

namespace epui::rpi::console {
namespace {

struct Candidate {
    std::string text;
    bool directory{false};
};

bool escaped_at(const char* text, std::size_t position) {
    std::size_t slashes = 0;
    while (position > 0 && text[--position] == '\\') ++slashes;
    return (slashes & 1u) != 0u;
}

bool shell_space(char ch) {
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

std::string unescape_token(const char* text, std::size_t length) {
    std::string result;
    result.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        if (text[i] == '\\' && i + 1 < length) ++i;
        result.push_back(text[i]);
    }
    return result;
}

bool needs_escape(char ch) {
    return shell_space(ch) || std::strchr("\\'\"$`!&;|<>()[]{}*?", ch) != nullptr;
}

std::string escape_token(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (char ch : text) {
        if (needs_escape(ch)) result.push_back('\\');
        result.push_back(ch);
    }
    return result;
}

void add_directory_candidates(const std::string& token,
                              std::vector<Candidate>& candidates) {
    const std::size_t slash = token.find_last_of('/');
    const std::string typed_directory = slash == std::string::npos
        ? std::string{} : token.substr(0, slash + 1);
    const std::string prefix = slash == std::string::npos
        ? token : token.substr(slash + 1);

    std::string search_directory = typed_directory.empty() ? "." : typed_directory;
    if (search_directory.compare(0, 2, "~/") == 0) {
        const char* home = std::getenv("HOME");
        if (!home || !*home) return;
        search_directory = std::string(home) + search_directory.substr(1);
    }

    std::error_code error;
    std::filesystem::directory_iterator iterator(
        search_directory, std::filesystem::directory_options::skip_permission_denied,
        error);
    const std::filesystem::directory_iterator end;
    while (!error && iterator != end) {
        const std::string name = iterator->path().filename().string();
        if (name.compare(0, prefix.size(), prefix) == 0
            && ((!prefix.empty() && prefix[0] == '.') || name.empty() || name[0] != '.')) {
            std::error_code status_error;
            const bool directory = iterator->is_directory(status_error);
            candidates.push_back({typed_directory + name,
                                  !status_error && directory});
        }
        iterator.increment(error);
    }
}

void scan_executable_directory(const std::string& directory,
                               const std::string& prefix,
                               std::vector<Candidate>& candidates) {
    std::error_code error;
    std::filesystem::directory_iterator iterator(
        directory.empty() ? "." : directory,
        std::filesystem::directory_options::skip_permission_denied, error);
    const std::filesystem::directory_iterator end;
    while (!error && iterator != end) {
        const std::string name = iterator->path().filename().string();
        if (name.compare(0, prefix.size(), prefix) == 0
            && ::access(iterator->path().c_str(), X_OK) == 0) {
            std::error_code status_error;
            if (!iterator->is_directory(status_error)) candidates.push_back({name, false});
        }
        iterator.increment(error);
    }
}

void add_command_candidates(const std::string& prefix,
                            std::vector<Candidate>& candidates) {
    const char* environment = std::getenv("PATH");
    const std::string paths = environment && *environment
        ? environment : "/usr/local/bin:/usr/bin:/bin";
    std::size_t begin = 0;
    while (begin <= paths.size()) {
        const std::size_t separator = paths.find(':', begin);
        scan_executable_directory(paths.substr(
            begin, separator == std::string::npos ? std::string::npos : separator - begin),
            prefix, candidates);
        if (separator == std::string::npos) break;
        begin = separator + 1;
    }
}

} // namespace

bool complete_shell_token(const char* command, std::size_t length,
                          std::size_t cursor, char* output,
                          std::size_t output_capacity,
                          std::size_t& output_length,
                          std::size_t& output_cursor) {
    if (!command || !output || output_capacity == 0 || cursor > length) return false;

    std::size_t token_begin = cursor;
    while (token_begin > 0) {
        const std::size_t position = token_begin - 1;
        if (shell_space(command[position]) && !escaped_at(command, position)) break;
        --token_begin;
    }
    std::size_t token_end = cursor;
    while (token_end < length
           && (!shell_space(command[token_end]) || escaped_at(command, token_end))) {
        ++token_end;
    }
    for (std::size_t i = token_begin; i < token_end; ++i) {
        if ((command[i] == '\'' || command[i] == '"') && !escaped_at(command, i)) return false;
    }

    const std::string token = unescape_token(command + token_begin, cursor - token_begin);
    if (token.empty()) return false;
    bool command_position = true;
    for (std::size_t i = 0; i < token_begin; ++i) {
        if (!shell_space(command[i])) {
            command_position = false;
            break;
        }
    }

    std::vector<Candidate> candidates;
    if (command_position && token.find('/') == std::string::npos) {
        add_command_candidates(token, candidates);
    } else {
        add_directory_candidates(token, candidates);
    }
    if (candidates.empty()) return false;

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& left, const Candidate& right) {
                  return left.text < right.text;
              });
    candidates.erase(std::unique(candidates.begin(), candidates.end(),
        [](const Candidate& left, const Candidate& right) {
            return left.text == right.text;
        }), candidates.end());

    std::string replacement = candidates.front().text;
    for (std::size_t i = 1; i < candidates.size(); ++i) {
        std::size_t common = 0;
        while (common < replacement.size() && common < candidates[i].text.size()
               && replacement[common] == candidates[i].text[common]) ++common;
        replacement.resize(common);
    }
    const char suffix = candidates.size() == 1
        ? (candidates.front().directory ? '/' : ' ') : 0;
    if (replacement == token && suffix == 0) return false;

    std::string escaped = escape_token(replacement);
    if (suffix != 0) escaped.push_back(suffix);
    const std::size_t suffix_size = length - token_end;
    const std::size_t completed_length = token_begin + escaped.size() + suffix_size;
    if (completed_length + 1 > output_capacity) return false;
    if (token_begin != 0) std::memcpy(output, command, token_begin);
    if (!escaped.empty()) std::memcpy(output + token_begin, escaped.data(), escaped.size());
    if (suffix_size != 0) {
        std::memcpy(output + token_begin + escaped.size(), command + token_end, suffix_size);
    }
    output[completed_length] = 0;
    output_length = completed_length;
    output_cursor = token_begin + escaped.size();
    return true;
}

} // namespace epui::rpi::console
