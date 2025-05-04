/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <initializer_list>
#include <ranges>
#include <source_location>
#include <stacktrace>
#include <string>
#include <string_view>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/env.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/print.hpp"
#include "tit/core/str.hpp"
#include "tit/core/sys.hpp"
#include "tit/core/tuple.hpp"
#include "tit/core/version.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void println_separator(char c) {
  println("{}", std::string(tty_width(TTY::Stdout), c));
}

void eprintln_separator(char c) {
  eprintln("{}", std::string(tty_width(TTY::Stderr), c));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

constexpr auto logo_lines = std::to_array<std::string_view>({
    R"(               ############               )",
    R"(          ######################          )",
    R"(        #######            #######        )",
    R"(      ######                  ######      )",
    R"(    #####          _,########._  #####    )",
    R"(   #####         .##############. #####   )",
    R"(  #####        .####"__'#########. #####  )",
    R"(  ####        _#### |_'| ##########.####  )",
    R"( ####      _-"``\"  `--  """'  `###; #### )",
    R"( ####     "--==="#.             `###.#### )",
    R"( ####          "###.         __.######### )",
    R"( ####           `####._ _.=######" "##### )",
    R"(  ####           ############"      ####  )",
    R"(  #####          #######'          #####  )",
    R"(   #####         #####'           #####   )",
    R"(    #####        `###'          #####     )",
    R"(      ######      `##         ######      )",
    R"(        #######    `#.     #######        )",
    R"(          ######################          )",
    R"(               ############               )",
});

void println_banner_lines(std::string_view product_name,
                          const std::vector<std::string>& info_lines) {
  if (product_name.empty()) product_name = "BlueTit Solver";
  const auto copyright_line =
      fmt_tm("© 2020 - %Y Oleg Butakov", str_to_tm("%F", version::commit_date));
  const auto product_lines = std::to_array<std::string_view>({
      product_name,
      "",
      copyright_line,
      "",
  });

  const auto total_lines = info_lines.size() + product_lines.size();
  TIT_ENSURE(total_lines <= logo_lines.size(), "Too many info lines.");
  const size_t padding = (logo_lines.size() - total_lines) / 2;

  println();
  println_separator('~');
  println();
  for (const auto& logo_line : logo_lines | std::views::take(padding)) {
    println("{}", logo_line);
  }
  for (const auto& [product_line, logo_line] : std::views::zip( //
           product_lines,
           logo_lines | std::views::drop(padding))) {
    println("{}   {}", logo_line, product_line);
  }
  for (const auto& [info_line, logo_line] : std::views::zip(
           info_lines,
           logo_lines | std::views::drop(padding + product_lines.size()))) {
    println("{}   {}", logo_line, info_line);
  }
  for (const auto& logo_line :
       logo_lines | std::views::drop(padding + total_lines)) {
    println("{}", logo_line);
  }
  println();
  println_separator('~');
  println();
}

void println_banner(std::string_view product_name,
                    std::initializer_list<pair_of_t<std::string>> info_pairs) {
  constexpr size_t min_key_width = 10;
  const size_t max_key_width = std::max(
      min_key_width,
      std::ranges::max(info_pairs | std::views::transform([](const auto& pair) {
                         return pair.first.size();
                       })));
  println_banner_lines(
      product_name,
      info_pairs | std::views::transform([max_key_width](const auto& pair) {
        return std::format("{} {:.>{}} {}",
                           pair.first,
                           "",
                           (max_key_width - pair.first.size()) + 4,
                           pair.second);
      }) | std::ranges::to<std::vector>());
}

} // namespace

void println_banner_and_system_info(std::string_view product_name) {
  // Skip the logo if requested. If logo is printed, set the variable to
  // prevent printing it again in the child processes.
  if (get_env("TIT_NO_BANNER", false)) return;
  set_env("TIT_NO_BANNER", true);

  println_banner(product_name,
                 {
                     {"Version", std::string{version::version}},
                     {"Commit", std::string{version::commit_hash}},
                     {"Host", host_name()},
                     {"OS", os_info()},
                     {"CPU", cpu_info()},
                     {"RAM", fmt_memsize(ram_size())},
                     {"Working dir", std::filesystem::current_path()},
                     {"Disk space", fmt_memsize(disk_space())},
                 });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void eprintln_crash_report(std::string_view message,
                           std::string_view cause,
                           std::string_view cause_description,
                           std::source_location loc,
                           std::stacktrace trace) {
  eprintln();
  eprintln();
  eprint("{}:{}:{}: {}", loc.file_name(), loc.line(), loc.column(), message);

  if (!cause.empty()) {
    eprintln();
    eprintln();
    eprintln("  {}", cause);
    if (!cause_description.empty()) {
      eprintln("  ^{:~>{}} {}", "", cause.size() - 1, cause_description);
    }
  }

  eprintln();
  eprintln();
  eprintln("Stack trace:");
  eprintln();
  eprintln("{}", trace);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
