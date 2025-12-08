/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <sys/poll.h>
#include <sys/types.h> // NOLINT(*-include-cleaner)

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// File descriptor type.
using fd_t = int;

/// File mode type.
using mode_t = ::mode_t; // NOLINT(*-include-cleaner)

/// Open a file descriptor.
/// @{
auto checked_open(const std::filesystem::path& file, int flags) -> fd_t;
auto checked_open(const std::filesystem::path& file, int flags, mode_t mode)
    -> fd_t;
/// @}

/// Close a file descriptor.
void checked_close(fd_t fd);

/// Read from a file descriptor.
/// @{
auto checked_read(fd_t fd, std::span<std::byte> buffer) -> size_t;
auto checked_read(fd_t fd, std::span<char> buffer) -> size_t;
/// @}

/// Write to a file descriptor.
/// @{
auto checked_write(fd_t fd, std::span<const std::byte> buffer) -> size_t;
auto checked_write(fd_t fd, std::span<const char> buffer) -> size_t;
/// @}

/// Duplicate a file descriptor.
void checked_dup2(fd_t fd, fd_t new_fd);

/// Get a file descriptor flags.
auto checked_get_fcntl(fd_t fd) -> int;

/// Set a file descriptor flags.
void checked_set_fcntl(fd_t fd, int flags);

/// Wait for file descriptors to become ready.
auto checked_poll( //
    std::span<pollfd> fds,
    std::chrono::milliseconds timeout = std::chrono::milliseconds{-1})
    -> size_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pipe descriptor type.
using pipe_t = std::array<fd_t, 2>;

/// Create a pipe.
auto checked_pipe() -> pipe_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Process ID type.
using pid_t = ::pid_t; // NOLINT(*-include-cleaner)

/// Fork a process.
auto checked_fork() -> pid_t;

/// Execute a file.
[[noreturn]] void checked_exec(const std::filesystem::path& file,
                               std::vector<std::string> args = {});

/// Send a signal to a process.
void checked_kill(pid_t pid, int sig);

/// Wait for a process to terminate.
auto checked_waitpid(pid_t pid, int& status, int options = 0) -> pid_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// File descriptor object.
class FD final {
public:

  /// Construct a file descriptor.
  explicit FD(fd_t fd = -1) noexcept;

  /// Move-construct a file descriptor.
  FD(FD&& other) noexcept;

  /// File descriptor is not copy-constructible.
  FD(const FD&) = delete;

  /// Move-assign a file descriptor.
  auto operator=(FD&& other) noexcept -> FD&;

  /// File descriptor is not copy-assignable.
  auto operator=(const FD&) -> FD& = delete;

  /// Close the file descriptor.
  ~FD();

  /// Get the file descriptor.
  explicit(false) operator fd_t() const noexcept;

  /// Reset the file descriptor.
  void reset(fd_t fd = -1);

private:

  fd_t fd_;

}; // class FD

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pipe object.
class Pipe final {
public:

  /// Construct a pipe object.
  explicit Pipe(pipe_t p = {-1, -1});

  /// Get the read end of the pipe.
  auto rend(this auto& self) noexcept -> auto& {
    return self.read_end_;
  }

  /// Get the write end of the pipe.
  auto wend(this auto& self) noexcept -> auto& {
    return self.write_end_;
  }

  /// Reset the pipe.
  void reset(pipe_t p = {-1, -1});

private:

  FD read_end_;
  FD write_end_;

}; // class Pipe

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Child process.
class Process final {
public:

  /// Callback for the forked process. Should not return.
  using ForkCallback = std::move_only_function<void()>;

  /// Callback for the process output events.
  using OutputCallback = std::move_only_function<void(std::string_view data)>;

  /// Callback for the process exit event.
  using ExitCallback = std::move_only_function<void(int code, int sig)>;

  /// Construct a child process object, but do not spawn it.
  Process() = default;

  /// Move-construct a child process object.
  Process(Process&& other) noexcept;

  /// Process object is not copy-constructible.
  Process(const Process&) = delete;

  /// Move-assign a child process object.
  auto operator=(Process&& other) noexcept -> Process&;

  /// Process object is not copy-assignable.
  auto operator=(const Process&) -> Process& = delete;

  /// Kill the process and destruct the object.
  ~Process();

  /// Get the child process ID.
  auto pid() const -> pid_t;

  /// Check if the process is running.
  auto is_running() const -> bool;

  /// Spawn the child process.
  /// @{
  void spawn_child(ForkCallback callback);
  void spawn_child(const std::filesystem::path& file,
                   std::vector<std::string> args = {});
  /// @}

  /// Send a signal to the child process.
  void kill_child(int sig = SIGTERM) const;

  /// Wait for the process to finish.
  void wait_child();

  /// Set the callback for the child process `stdout` data.
  void on_stdout(OutputCallback callback);

  /// Set the callback for the child process `stderr` data.
  void on_stderr(OutputCallback callback);

  /// Set the callback for the child process exit event.
  void on_exit(ExitCallback callback);

private:

  void call_on_stdout_(std::string_view data);
  void call_on_stderr_(std::string_view data);
  void call_on_exit_(int code, int sig);

  pid_t pid_ = -1;
  Pipe stdout_pipe_;
  Pipe stderr_pipe_;
  OutputCallback stdout_callback_;
  OutputCallback stderr_callback_;
  ExitCallback exit_callback_;

}; // class Process

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
