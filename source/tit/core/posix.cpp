/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <cerrno>
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
  TIT_ALWAYS_ASSERT((flags & O_CREAT) == 0,
                    "Call to `checked_open` with `O_CREAT` flag must be "
                    "specified accompanied with a non-zero `mode` argument.");

  const auto fd = open(file.c_str(), flags); // NOLINT(*-vararg)
  TIT_ENSURE_ERRNO(fd >= 0, "Failed to open a file.");

  return fd;
}
auto checked_open(const std::filesystem::path& file, int flags, mode_t mode)
    -> fd_t {
  TIT_ALWAYS_ASSERT((flags & O_CREAT) != 0,
                    "Call to `checked_open` with `O_CREAT` flag must be "
                    "specified accompanied with a non-zero `mode` argument.");
  TIT_ASSERT(mode != 0, "Invalid file mode!");

  const auto fd = open(file.c_str(), flags, mode); // NOLINT(*-vararg)
  TIT_ENSURE_ERRNO(fd >= 0, "Failed to open a file.");

  return fd;
}

void checked_close(fd_t fd) {
  TIT_ASSERT(fd >= 0, "Invalid file descriptor!");

  const auto status = close(fd);
  TIT_ENSURE_ERRNO(status == 0, "Failed to close a file descriptor.");
}

auto checked_read(fd_t fd, std::span<std::byte> buffer) -> size_t {
  TIT_ASSERT(fd >= 0, "Invalid file descriptor!");

  const auto num_bytes = read(fd, buffer.data(), buffer.size());
  TIT_ENSURE_ERRNO(num_bytes >= 0, "Failed to read from a file descriptor.");

  return static_cast<size_t>(num_bytes);
}
auto checked_read(fd_t fd, std::span<char> buffer) -> size_t {
  return checked_read(
      fd,
      std::span{safe_bit_ptr_cast<std::byte*>(buffer.data()), buffer.size()});
}

auto checked_write(fd_t fd, std::span<const std::byte> buffer) -> size_t {
  TIT_ASSERT(fd >= 0, "Invalid file descriptor!");

  const auto num_bytes = write(fd, buffer.data(), buffer.size());
  TIT_ENSURE_ERRNO(num_bytes >= 0, "Failed to write to a file descriptor.");

  return static_cast<size_t>(num_bytes);
}
auto checked_write(fd_t fd, std::span<const char> buffer) -> size_t {
  return checked_write(
      fd,
      std::span{safe_bit_ptr_cast<const std::byte*>(buffer.data()),
                buffer.size()});
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
  // NOLINTEND(*-vararg)

  return flags;
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

  const auto num_events =
      poll(fds.data(), fds.size(), static_cast<int>(timeout.count()));
  TIT_ENSURE_ERRNO(num_events >= 0, "Failed to poll file descriptors.");

  return static_cast<size_t>(num_events);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto checked_pipe() -> pipe_t {
  pipe_t result{-1, -1};

  const auto status = pipe(result.data());
  TIT_ENSURE_ERRNO(status == 0, "Failed to create a pipe.");

  return result;
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

namespace {

// `EINTR` can be returned by a system call, if the call was interrupted by
// a signal. We need to retry the call in this case.
void retry_on_eintr(auto func) {
  while (true) {
    try {
      return func();
    } catch (const ErrnoException& e) {
      if (e.errno_value() != EINTR) throw;
    }
  }
};

// `EAGAIN` (or `EWOULDBLOCK`) can be returned by a system call, if the call
// would block. We need to treat this as if no data was read, but the FD is
// still open.
auto checked_read_or_would_block(fd_t fd,
                                 std::span<char> buf,
                                 size_t& num_bytes) -> bool {
  while (true) {
    try {
      num_bytes = checked_read(fd, buf);
      return true;
    } catch (const ErrnoException& e) {
      const auto ev = e.errno_value();
      if (ev == EINTR) continue;
      if (ev == EAGAIN) return false;
#if EAGAIN != EWOULDBLOCK
      if (ev == EWOULDBLOCK) return false;
#endif
      throw;
    }
  }
};

} // namespace

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

    // Note: `std::move_only_function::operator=(std::move_only_function&&)`
    //       is not `noexcept`.
    terminate_on_exception([this, &other] {
      stdout_callback_ = std::move(other.stdout_callback_);
      stderr_callback_ = std::move(other.stderr_callback_);
      exit_callback_ = std::move(other.exit_callback_);
    });
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
    retry_on_eintr([&fds] { checked_poll(fds); });

    if (size_t num_bytes = 0;
        stdout_is_open && fds[0].revents != 0 &&
        checked_read_or_would_block(stdout_pipe_.rend(), buffer, num_bytes)) {
      if (num_bytes != 0) {
        call_on_stdout_({buffer.data(), num_bytes});
      } else {
        stdout_is_open = false;
        stdout_pipe_.rend().reset();
      }
    }

    if (size_t num_bytes = 0;
        stderr_is_open && fds[1].revents != 0 &&
        checked_read_or_would_block(stderr_pipe_.rend(), buffer, num_bytes)) {
      if (num_bytes != 0) {
        call_on_stderr_({buffer.data(), num_bytes});
      } else {
        stderr_is_open = false;
        stderr_pipe_.rend().reset();
      }
    }
  }

  int status = 0;
  retry_on_eintr([pid = pid_, &status] { checked_waitpid(pid, status); });

  const auto code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
  const auto sig = WIFSIGNALED(status) ? WTERMSIG(status) : 0;
  call_on_exit_(code, sig);

  pid_ = -1;
}

void Process::on_stdout(OutputCallback callback) {
  stdout_callback_ = std::move(callback);
}

void Process::on_stderr(OutputCallback callback) {
  stderr_callback_ = std::move(callback);
}

void Process::on_exit(ExitCallback callback) {
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
