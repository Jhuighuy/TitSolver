/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <signal.h> // NOLINT(*-deprecated-headers)
#include <sys/poll.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <sys/fcntl.h>
#include <sys/signal.h>
#elifdef __linux__
#include <fcntl.h>
#include <stdlib.h> // NOLINT(*-deprecated-headers)
#endif

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/posix.hpp"
#include "tit/core/type.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto checked_open(const std::filesystem::path& file, int flags) -> fd_t {
  const auto fd = open(file.c_str(), flags); // NOLINT(*-vararg)
  TIT_ENSURE_ERRNO(fd >= 0, "Failed to open a file.");
  return fd;
}

void checked_close(fd_t fd) {
  TIT_ASSERT(fd >= 0, "Invalid file descriptor!");
  const auto status = close(fd);
  TIT_ENSURE_ERRNO(status == 0, "Failed to close a file descriptor.");
}

auto checked_read(fd_t fd, std::span<std::byte> buf) -> size_t {
  TIT_ASSERT(fd >= 0, "Invalid file descriptor!");
  const auto status = read(fd, buf.data(), buf.size());
  TIT_ENSURE_ERRNO(status >= 0, "Failed to read from a file descriptor.");
  return static_cast<size_t>(status);
}
auto checked_read(fd_t fd, std::span<char> buf) -> size_t {
  return checked_read(
      fd,
      std::span{safe_bit_ptr_cast<std::byte*>(buf.data()), buf.size()});
}

auto checked_write(fd_t fd, std::span<const std::byte> buf) -> size_t {
  TIT_ASSERT(fd >= 0, "Invalid file descriptor!");
  const auto status = write(fd, buf.data(), buf.size());
  TIT_ENSURE_ERRNO(status >= 0, "Failed to write to a file descriptor.");
  return static_cast<size_t>(status);
}
auto checked_write(fd_t fd, std::span<const char> buf) -> size_t {
  return checked_write(
      fd,
      std::span{safe_bit_ptr_cast<const std::byte*>(buf.data()), buf.size()});
}

void checked_dup2(fd_t fd, fd_t new_fd) {
  TIT_ASSERT(fd >= 0, "Invalid file descriptor!");
  TIT_ASSERT(new_fd >= 0, "Invalid file descriptor!");
  const auto status = dup2(fd, new_fd);
  TIT_ENSURE_ERRNO(status >= 0, "`dup2` failed.");
}

auto checked_get_fcntl(fd_t fd) -> int {
  TIT_ASSERT(fd >= 0, "Invalid file descriptor!");
  // NOLINTBEGIN(*-vararg)
  const int flags = fcntl(fd, F_GETFL, 0);
  TIT_ENSURE_ERRNO(flags >= 0, "Failed to get a file descriptor flags.");
  return flags;
  // NOLINTEND(*-vararg)
}

void checked_set_fcntl(fd_t fd, int flags) {
  TIT_ASSERT(fd >= 0, "Invalid file descriptor!");
  // NOLINTBEGIN(*-vararg)
  const auto status = fcntl(fd, F_SETFL, flags);
  TIT_ENSURE_ERRNO(status == 0, "Failed to set a file descriptor flags.");
  // NOLINTEND(*-vararg)
}

auto checked_poll(std::span<pollfd> fds, std::chrono::milliseconds timeout)
    -> size_t {
  TIT_ASSERT(!fds.empty(), "Invalid file descriptor array!");
  TIT_ASSERT(is_safe_cast<int>(timeout.count()), "Invalid timeout!");
  const auto status =
      poll(fds.data(), fds.size(), static_cast<int>(timeout.count()));
  TIT_ENSURE_ERRNO(status >= 0, "Failed to poll file descriptors.");
  return static_cast<size_t>(status);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto checked_pipe() -> pipe_t {
  pipe_t p{-1, -1};
  const auto status = pipe(p.data());
  TIT_ENSURE_ERRNO(status == 0, "Failed to create a pipe.");
  return p;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto checked_fork() -> pid_t {
  const auto pid = fork();
  TIT_ENSURE_ERRNO(pid >= 0, "Failed to fork.");
  return pid;
}

[[noreturn]] void checked_exec(const std::filesystem::path& file,
                               std::vector<std::string> args) {
  /// @todo In C++26 there would be no need for `.string()`.
  TIT_ENSURE(std::filesystem::is_regular_file(file),
             "Cannot execute not a regular file '{}'.",
             file.string());
  auto file_str = file.string();

  std::vector<char*> argv{};
  argv.reserve(args.size() + 2);
  argv.push_back(file_str.data());
  for (auto& arg : args) argv.push_back(arg.data());
  argv.push_back(nullptr);

  execvp(file_str.c_str(), argv.data());
  TIT_THROW_ERRNO("`execvp` failed.");
}

void checked_kill(pid_t pid, int sig) {
  TIT_ASSERT(pid > 0, "Invalid process ID!");
  const auto status = kill(pid, sig);
  TIT_ENSURE_ERRNO(status == 0, "Failed to kill a process.");
}

auto checked_waitpid(pid_t pid, int& status, int options) -> pid_t {
  TIT_ASSERT(pid > 0, "Invalid process ID!");
  const auto result = waitpid(pid, &status, options);
  TIT_ENSURE_ERRNO(result >= 0, "Failed to wait for a process.");
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

FD::FD(fd_t fd) noexcept : fd_{fd} {
  TIT_ASSERT(fd >= -1, "Invalid file descriptor!");
}

FD::FD(FD&& other) noexcept : FD{std::exchange(other.fd_, -1)} {}

auto FD::operator=(FD&& other) noexcept -> FD& {
  if (this != &other) {
    terminate_on_exception(
        [this, &other] { reset(std::exchange(other.fd_, -1)); });
  }

  return *this;
}

FD::~FD() {
  terminate_on_exception([this] { reset(); });
}

FD::operator fd_t() const noexcept {
  return fd_;
}

void FD::reset(fd_t fd) {
  if (fd_ != -1) checked_close(fd_);
  TIT_ASSERT(fd >= -1, "Invalid file descriptor!");
  fd_ = fd;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Pipe::Pipe(pipe_t p) {
  read_end_.reset(p[0]);
  write_end_.reset(p[1]);
}

void Pipe::reset(pipe_t p) {
  read_end_.reset(p[0]);
  write_end_.reset(p[1]);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Process::Process(Process&& other) noexcept
    : pid_{std::exchange(other.pid_, -1)},
      stdout_pipe_{std::move(other.stdout_pipe_)},
      stderr_pipe_{std::move(other.stderr_pipe_)},
      stdout_callback_{std::move(other.stdout_callback_)},
      stderr_callback_{std::move(other.stderr_callback_)},
      exit_callback_{std::move(other.exit_callback_)} {}

auto Process::operator=(Process&& other) noexcept -> Process& {
  if (this != &other) {
    terminate_on_exception([this] {
      if (pid_ == -1) return;
      kill_child(SIGKILL);
      wait_child();
    });

    pid_ = std::exchange(other.pid_, -1);
    stdout_pipe_ = std::move(other.stdout_pipe_);
    stderr_pipe_ = std::move(other.stderr_pipe_);
    stdout_callback_ = std::move(other.stdout_callback_);
    stderr_callback_ = std::move(other.stderr_callback_);
    exit_callback_ = std::move(other.exit_callback_);
  }

  return *this;
}

Process::~Process() {
  terminate_on_exception([this] {
    if (pid_ == -1) return;
    kill_child(SIGKILL);
    wait_child();
  });
}

auto Process::pid() const -> pid_t {
  return pid_;
}

auto Process::is_running() const -> bool {
  return pid_ != -1;
}

void Process::spawn_child(ForkCallback callback) {
  TIT_ALWAYS_ASSERT(pid_ == -1, "Process is already running!");

  stdout_pipe_.reset(checked_pipe());
  stderr_pipe_.reset(checked_pipe());

  pid_ = checked_fork();
  if (pid_ == 0) {
    checked_dup2(stdout_pipe_.wend(), STDOUT_FILENO);
    stdout_pipe_.reset();

    checked_dup2(stderr_pipe_.wend(), STDERR_FILENO);
    stderr_pipe_.reset();

    callback();

    std::unreachable();
  }

  stdout_pipe_.wend().reset();
  checked_set_fcntl(stdout_pipe_.rend(),
                    checked_get_fcntl(stdout_pipe_.rend()) | O_NONBLOCK);

  stderr_pipe_.wend().reset();
  checked_set_fcntl(stderr_pipe_.rend(),
                    checked_get_fcntl(stderr_pipe_.rend()) | O_NONBLOCK);
}

void Process::spawn_child(const std::filesystem::path& file,
                          std::vector<std::string> args) {
  spawn_child([&file, &args] { checked_exec(file, std::move(args)); });
}

void Process::kill_child(int sig) const {
  TIT_ALWAYS_ASSERT(pid_ != -1, "Process is not running!");

  checked_kill(pid_, sig);
}

void Process::wait_child() {
  TIT_ALWAYS_ASSERT(pid_ != -1, "Process is not running!");

  std::array<char, 4096> buffer{};

  auto fds = std::to_array<pollfd>({
      {
          .fd = stdout_pipe_.rend(),
          .events = POLLIN,
          .revents = 0,
      },
      {
          .fd = stderr_pipe_.rend(),
          .events = POLLIN,
          .revents = 0,
      },
  });

  bool stdout_is_open = true;
  bool stderr_is_open = true;
  while (stdout_is_open || stderr_is_open) {
    checked_poll(fds);

    if (stdout_is_open && fds[0].revents != 0) {
      if (const auto n = checked_read(stdout_pipe_.rend(), buffer); n != 0) {
        call_on_stdout_({buffer.data(), n});
      } else {
        stdout_is_open = false;
        stdout_pipe_.rend().reset();
      }
    }

    if (stderr_is_open && fds[1].revents != 0) {
      if (const auto n = checked_read(stderr_pipe_.rend(), buffer); n != 0) {
        call_on_stderr_({buffer.data(), n});
      } else {
        stderr_is_open = false;
        stderr_pipe_.rend().reset();
      }
    }
  }

  int status = 0;
  checked_waitpid(pid_, status);

  const auto code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
  const auto sig = WIFSIGNALED(status) ? WTERMSIG(status) : 0;
  call_on_exit_(code, sig);

  pid_ = -1;
}

void Process::on_stdout(OutputCallback callback) noexcept {
  stdout_callback_ = std::move(callback);
}

void Process::on_stderr(OutputCallback callback) noexcept {
  stderr_callback_ = std::move(callback);
}

void Process::on_exit(ExitCallback callback) noexcept {
  exit_callback_ = std::move(callback);
}

void Process::call_on_stdout_(std::string_view data) {
  if (stdout_callback_) stdout_callback_(data);
}

void Process::call_on_stderr_(std::string_view data) {
  if (stderr_callback_) stderr_callback_(data);
}

void Process::call_on_exit_(int code, int sig) {
  if (exit_callback_) exit_callback_(code, sig);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
