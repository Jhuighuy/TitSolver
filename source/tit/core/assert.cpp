/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <regex>
#include <source_location>
#include <string>
#include <string_view>

#include <cxxabi.h>
#include <execinfo.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include "tit/core/assert.hpp"
#include "tit/core/string_utils.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

auto try_demangle_name(std::string& name) -> bool {
  int status = 0;
  auto* const demangled_name =
      abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status);
  if (status == 0 && demangled_name != nullptr) {
    // Write back the demangled name.
    name = demangled_name;
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,*-no-malloc)
    std::free(demangled_name);
    return true;
  }
  // Leave mangled name.
  return false;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

inline constexpr auto max_name_length = 59ZU;

inline constexpr auto error_style =
    fmt::emphasis::bold | fmt::fg(fmt::color::red);
inline constexpr auto message_style =
    fmt::emphasis::bold | fmt::fg(fmt::color::purple);
inline constexpr auto note_style = fmt::emphasis::italic;
inline constexpr auto keyword_style = fmt::fg(fmt::color::blue);

// Prettify demangled name.
void prettify_demangled_name(std::string& name) {
  // Highlight keywords.
  constexpr auto keywords = std::array{
      "auto",  "void",     "bool",     "char",     "wchar_t", "int",
      "float", "double",   "signed",   "unsigned", "short",   "long",
      "const", "volatile", "operator", "template",
  };
  static const std::regex keyword_regex(
      "\\b(" + string_utils::join("|", keywords) + ")\\b");
  static const auto keyword_sub = fmt::format(keyword_style, "$&");
  name = std::regex_replace(name, keyword_regex, keyword_sub);
  // Shrink lambda mentions.
  static const std::regex lambda_regex(R"(\{lambda[^\{\}]+\#(\d+)})");
  static const auto lambda_sub = fmt::format(note_style, "lambda#$1");
  name = std::regex_replace(name, lambda_regex, lambda_sub);
  // While the function is too long, fold all the template arguments.
  static const std::regex template_regex("<[^<>]+>");
  static constexpr auto template_sub = "⟨…⟩";
  while (name.size() > max_name_length) {
    if (std::smatch match{}; !std::regex_search(name, match, template_regex)) {
      break;
    }
    name = std::regex_replace(name, template_regex, template_sub);
  }
}

// Prettify mangled name (as possible).
void prettify_mangled_name(std::string& name) {
  // Mangled names are hard to read and do not provide much sensible
  // information. So just trim them.
  if (name.size() <= max_name_length) return;
  name = name.substr(0, max_name_length - 1) + "…";
}

// Try to demangle and prettify the result.
void prettify_name(std::string& name) {
  if (try_demangle_name(name)) prettify_demangled_name(name);
  else prettify_mangled_name(name);
}

// Print beautiful call stack entry.
void print_call_stack_symbol(const std::string& symbol) {
  static const std::regex bsd_style(
      R"(\d+\s+([^\s]+)\s+0x[0-9A-Fa-f]+\s+([^\s]+)\s+\+\s+\d+)");
  std::string object_name{}, function_name{};
  if (std::smatch match{}; std::regex_match(symbol, match, bsd_style)) {
    object_name = match.str(1);
    function_name = match.str(2);
  } else {
    // Unknown style, fallback to default.
    fmt::print(stderr, "{}", symbol);
    return;
  }
  // Prettify function name.
  prettify_name(function_name);
  // Print what we've got.
  fmt::print(stderr, "{:<12}{} {}\n",
             fmt::styled(object_name, fmt::emphasis::bold),
             fmt::styled("@", note_style), function_name);
}

// Print beautiful call stack.
__attribute__((noinline)) void print_call_stack() {
  // Get the stack trace.
  constexpr int max_stack_depth = 50;
  std::array<void*, max_stack_depth> stack_trace{};
  const auto stack_depth = backtrace(stack_trace.data(), max_stack_depth);
  auto** const symbols = backtrace_symbols(stack_trace.data(), stack_depth);
  // Print the call stack (if available).
  fmt::print(stderr, fmt::emphasis::bold, "note: ");
  if (symbols == nullptr) {
    fmt::print(stderr, "no call stack is available.\n");
  } else {
    fmt::print(stderr, "call stack:\n");
    for (int i = /*skip current function*/ 1; i < stack_depth; ++i) {
      // Symbol contains a stack trace entry description in unspecified format.
      auto* const symbol = symbols[i];
      if (symbol == nullptr) continue;
      fmt::print(stderr, "[{:>2d}] ", fmt::styled(i, message_style));
      print_call_stack_symbol(symbol);
    }
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,*-no-malloc)
    free(symbols);
  }
}

// Print beautiful error message.
void print_pretty_error_message(std::string_view expression,
                                std::string_view message,
                                std::source_location location) {
  fmt::print(stderr, fmt::emphasis::bold, "{}:{}:{}: ", //
             location.file_name(), location.line(), location.column());
  fmt::print(stderr, error_style, "internal consistency check failed:\n\n");
  fmt::print(stderr, error_style, "    {}\n", expression);
  fmt::print(stderr, message_style, "    ^{:~>{}} {}\n\n", "",
             expression.size() - 1, message);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Abort the current process in non-`constexpr` context.
[[noreturn]] void _ensure_failed(std::string_view expression,
                                 std::string_view message,
                                 std::source_location location) noexcept {
  // Make sure only the first failed gets reported.
  static std::mutex mutex{};
  mutex.lock();
  try {
    // Report the failure.
    print_pretty_error_message(expression, message, location);
    print_call_stack();
  } catch (...) {} // NOLINT(bugprone-empty-catch)
  // Terminate.
  std::fflush(stderr); // NOLINT(cert-err33-c)
  std::abort();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
