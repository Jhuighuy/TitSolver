/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <ranges>
#include <source_location>
#include <stacktrace>
#include <string>
#include <string_view>

#include "tit/core/checks.hpp"
#include "tit/core/opencl.hpp"
#include "tit/core/print.hpp"
#include "tit/core/str.hpp"
#include "tit/core/sys.hpp"
#include "tit/core/version.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void println_separator(char c) {
  println("{}", std::string(terminal_width(stdout_fd), c));
}

void eprintln_separator(char c) {
  eprintln("{}", std::string(terminal_width(stderr_fd), c));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void println_logo_and_system_info() {
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

  const auto info_lines = std::to_array<std::string>({
      "BlueTit Solver",
      "",
      fmt_tm("© 2020 - %Y Oleg Butakov", str_to_tm("%F", version::commit_date)),
      "",
      std::format("Version ........ {}", version::version),
      std::format("Commit ......... {}", version::commit_hash),
      std::format("Host ........... {}", host_name()),
      std::format("OS ............. {}", os_info()),
      std::format("CPU ............ {}", cpu_info()),
      std::format("RAM ............ {}", fmt_memsize(ram_size())),
      std::format("OpenCL ......... {}", ocl::Platform::default_().info()),
      std::format("Working dir .... {}", std::filesystem::current_path()),
      std::format("Disk space ..... {}", fmt_memsize(disk_space())),
  });

  TIT_ASSERT(info_lines.size() <= logo_lines.size(), "Too many lines.");
  const auto padding = (logo_lines.size() - info_lines.size()) / 2;
  std::array<std::string_view, logo_lines.size()> logo_lines_padded;
  std::ranges::copy(info_lines, logo_lines_padded.begin() + padding);

  println();
  println_separator('~');
  println();
  for (const auto& [logo_line, info_line] :
       std::views::zip(logo_lines, logo_lines_padded)) {
    println("{}   {}", logo_line, info_line);
  }
  println();
  println_separator('~');
  println();
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
