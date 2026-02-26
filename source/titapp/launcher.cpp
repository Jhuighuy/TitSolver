/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <mach-o/dyld.h>
#include <objc/message.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <unistd.h>

namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//
// Get the path to the current executable.
//
auto proc_path() -> std::filesystem::path {
  std::uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  if (size == 0) {
    throw std::runtime_error(
        "Internal error: failed to get the executable path size.");
  }

  std::string result(size, '\0');
  if (_NSGetExecutablePath(result.data(), &size) != 0) {
    throw std::runtime_error(
        "Internal error: failed to get the executable path.");
  }

  // `size` might be larger than the actual path length.
  result.resize(std::strlen(result.data()));

  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//
// Launch the application.
//
[[noreturn]] void launch(int argc, char** argv) {
  // Locate the installation root directory.
  const auto launcher_path = std::filesystem::weakly_canonical(proc_path());
  const auto launcher_dir = launcher_path.parent_path();
  const auto install_root = std::filesystem::absolute(
      launcher_dir.parent_path().parent_path().parent_path());

  // Locate the application executable.
  const auto app_path = install_root / "bin" / "titapp";
  auto app_path_str = app_path.string();

  // Prepare the arguments.
  std::vector<char*> child_argv;
  child_argv.reserve(static_cast<std::size_t>(argc) + 1);
  child_argv.push_back(app_path_str.data());
  for (int i = 1; i < argc; i++) child_argv.push_back(argv[i]);
  child_argv.push_back(nullptr);

  // Execute the application.
  execv(child_argv.front(), child_argv.data());

  // If we reach this point, it means that the `execv()` failed.
  throw std::runtime_error(std::format(
      "Failed to execute '{}'.\n"
      "\n"
      "{}.\n"
      "\n"
      "Please make sure that the application is correctly installed.",
      app_path_str,
      std::strerror(errno))); // NOLINT(*-mt-unsafe)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// `objc_msgSend` is declared without arguments, so we need to cast it to
// a properly-typed function pointer.
template<class Ret, class... Args>
auto send(Args... args) noexcept -> Ret {
  // NOLINTNEXTLINE(*-reinterpret-cast)
  return reinterpret_cast<Ret (*)(Args...)>(&objc_msgSend)(args...);
};

//
// Show a dialog to the user.
//
void alert(const char* title, const char* message) noexcept {
  // Prepare the classes.
  static Class NSAlert = objc_getClass("NSAlert");
  static Class NSAutoreleasePool = objc_getClass("NSAutoreleasePool");
  static Class NSString = objc_getClass("NSString");

  // Prepare the selectors.
  static SEL addButtonWithTitle = sel_registerName("addButtonWithTitle:");
  static SEL alloc = sel_registerName("alloc");
  static SEL drain = sel_registerName("drain");
  static SEL init = sel_registerName("init");
  static SEL runModal = sel_registerName("runModal");
  static SEL setInformativeText = sel_registerName("setInformativeText:");
  static SEL setMessageText = sel_registerName("setMessageText:");
  static SEL stringWithUTF8String = sel_registerName("stringWithUTF8String:");

  // NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  id pool = send<id>(send<id>(NSAutoreleasePool, alloc), init);

  {
    // NSAlert* alert = [[NSAlert alloc] init];
    id alert = send<id>(send<id>(NSAlert, alloc), init);

    // [alert setMessageText: [NSString stringWithUTF8String: title]];
    send<void>(alert,
               setMessageText,
               send<id>(NSString, stringWithUTF8String, title));

    // [alert setInformativeText: [NSString stringWithUTF8String: message]];
    send<void>(alert,
               setInformativeText,
               send<id>(NSString, stringWithUTF8String, message));

    // [alert addButtonWithTitle: @"OK"];
    send<id>(alert,
             addButtonWithTitle,
             send<id>(NSString, stringWithUTF8String, "OK"));

    // [alert runModal];
    send<int>(alert, runModal);
  }

  // [pool drain];
  send<void>(pool, drain);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

//
// Entry point.
//
auto main(int argc, char** argv) -> int {
  try {
    launch(argc, argv);
  } catch (const std::exception& e) {
    alert("Failed to launch BlueTit Solver", e.what());
    return -1;
  }

  // We should never reach this point.
  std::unreachable();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
