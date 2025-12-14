/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <string>
#include <thread>
#include <utility>

#include <sys/poll.h>
#include <unistd.h>
#ifdef __APPLE__
#include <sys/fcntl.h>
#include <sys/wait.h>
#elifdef __linux__
#include <fcntl.h>
#include <stdlib.h> // NOLINT(*-deprecated-headers)
#endif

// NOLINTBEGIN(*-use-after-move,clang-analyzer-cplusplus.Move)

#include "tit/core/exception.hpp"
#include "tit/core/main.hpp"
#include "tit/core/posix.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

constexpr auto invalid = std::numeric_limits<int>::max();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("checked_open/checked_close") {
  SUBCASE("success") {
    const auto fd = checked_open(__FILE__, O_RDONLY);
    CHECK(fd >= 2);
    checked_close(fd);
  }
  SUBCASE("failure") {
    SUBCASE("invalid path") {
      SUBCASE("open") {
        CHECK_THROWS_MSG(checked_open("/invalid/path", O_RDONLY),
                         ErrnoException,
                         "Failed to open a file.");
      }
      SUBCASE("create") {
        CHECK_THROWS_MSG(checked_open("/invalid/path", O_CREAT, 0666),
                         ErrnoException,
                         "Failed to open a file.");
      }
    }
    SUBCASE("invalid flags") {
      SUBCASE("open") {
        CHECK_THROWS_MSG(checked_open(__FILE__, -1 ^ O_CREAT),
                         ErrnoException,
                         "Failed to open a file.");
      }
      SUBCASE("create") {
        CHECK_THROWS_MSG(checked_open(__FILE__, -1 | O_CREAT, 0666),
                         ErrnoException,
                         "Failed to open a file.");
      }
    }
    SUBCASE("invalid file descriptor") {
      CHECK_THROWS_MSG(checked_close(invalid),
                       ErrnoException,
                       "Failed to close a file descriptor.");
    }
  }
}

TEST_CASE("checked_read") {
  SUBCASE("success") {
    const std::string file_name = "test.txt";
    const std::string message = "Hello, World!";
    std::ofstream{file_name} << message;

    const auto fd = checked_open(file_name, O_RDONLY);

    std::array<char, 20> buffer{};
    CHECK(checked_read(fd, buffer) == message.size());
    CHECK(std::string_view{buffer.data(), message.size()} == message);

    checked_close(fd);
  }
  SUBCASE("failure") {
    SUBCASE("invalid descriptor") {
      std::array<char, 20> buffer{};
      CHECK_THROWS_MSG(checked_read(invalid, buffer),
                       ErrnoException,
                       "Failed to read from a file descriptor.");
    }
  }
}

TEST_CASE("checked_write") {
  SUBCASE("success") {
    const std::string file_name = "test.txt";
    const std::string message = "Hello, World!";

    const auto fd = checked_open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    CHECK(checked_write(fd, std::span{message.data(), message.size()}) ==
          message.size());

    checked_close(fd);

    std::string content;
    std::getline(std::ifstream{file_name}, content);
    CHECK(content == message);
  }
  SUBCASE("failure") {
    SUBCASE("invalid descriptor") {
      CHECK_THROWS_MSG(checked_write(invalid, "Hello, World!"),
                       ErrnoException,
                       "Failed to write to a file descriptor.");
    }
  }
}

TEST_CASE("checked_dup2") {
  SUBCASE("success") {
    const std::string file_name = "test.txt";
    const std::string message = "Hello, World!";
    std::ofstream{file_name} << message;

    const auto old_fd = checked_open(file_name, O_RDONLY);

    const fd_t new_fd = checked_open(__FILE__, O_RDONLY);
    checked_close(new_fd);

    checked_dup2(old_fd, new_fd);

    std::array<char, 20> buffer{};
    CHECK(checked_read(new_fd, buffer) == message.size());
    CHECK(std::string_view{buffer.data(), message.size()} == message);

    checked_close(old_fd);
    checked_close(new_fd);
  }
  SUBCASE("failure") {
    SUBCASE("invalid descriptor") {
      CHECK_THROWS_MSG(checked_dup2(invalid, invalid),
                       ErrnoException,
                       "`dup2` failed.");
    }
  }
}

TEST_CASE("checked_get_fcntl") {
  SUBCASE("success") {
    const auto fd = checked_open("test.txt", O_WRONLY | O_CREAT, 0666);
    CHECK((checked_get_fcntl(fd) & O_WRONLY) == O_WRONLY);
    checked_close(fd);
  }
  SUBCASE("failure") {
    SUBCASE("invalid descriptor") {
      CHECK_THROWS_MSG(checked_get_fcntl(invalid),
                       ErrnoException,
                       "Failed to get a file descriptor flags.");
    }
  }
}

TEST_CASE("checked_set_fcntl") {
  SUBCASE("success") {
    const auto fd = checked_open(__FILE__, O_RDONLY);
    checked_set_fcntl(fd, O_RDONLY | O_NONBLOCK);
    CHECK((checked_get_fcntl(fd) & O_NONBLOCK) != 0);
    checked_close(fd);
  }
  SUBCASE("failure") {
    SUBCASE("invalid descriptor") {
      CHECK_THROWS_MSG(checked_set_fcntl(invalid, O_RDONLY | O_NONBLOCK),
                       ErrnoException,
                       "Failed to set a file descriptor flags.");
    }
  }
}

TEST_CASE("checked_poll") {
  SUBCASE("success") {
    SUBCASE("ready descriptor") {
      const auto fd = checked_open(__FILE__, O_RDONLY);
      std::array<pollfd, 1> fds{};
      fds[0].fd = fd;
      fds[0].events = POLLIN;

      CHECK(checked_poll(fds, std::chrono::milliseconds{0}) == 1);
      CHECK(fds[0].revents == POLLIN);

      checked_close(fd);
    }
    SUBCASE("timeout") {
      const auto p = checked_pipe();

      std::array<pollfd, 1> fds{};
      fds[0].fd = p[0];
      fds[0].events = POLLIN;

      CHECK(checked_poll(fds, std::chrono::milliseconds{10}) == 0);

      checked_close(p[0]);
      checked_close(p[1]);
    }
    SUBCASE("invalid descriptor") {
      std::array<pollfd, 1> fds{};
      fds[0].fd = invalid;
      fds[0].events = POLLIN;

      CHECK(checked_poll(fds, std::chrono::milliseconds{0}) == 1);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("checked_pipe") {
  const auto p = checked_pipe();

  CHECK(p[0] >= 2);
  CHECK(p[1] >= 2);
  CHECK(p[0] != p[1]);

  const std::string message = "Hello, World!";
  CHECK(checked_write(p[1], message) == message.size());

  std::array<char, 20> buffer{};
  CHECK(checked_read(p[0], buffer) == message.size());
  CHECK(std::string_view{buffer.data(), message.size()} == message);

  checked_close(p[0]);
  checked_close(p[1]);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("checked_fork/checked_waitpid") {
  SUBCASE("success") {
    constexpr auto exit_code = 13;
    const auto pid = checked_fork();
    if (pid == 0) fast_exit(exit_code);

    REQUIRE(pid > 0);

    int status = 0;
    CHECK(checked_waitpid(pid, status, 0) == pid);

    CHECK(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == exit_code);
  }
  SUBCASE("failure") {
    SUBCASE("invalid PID") {
      int status = 0;
      CHECK_THROWS_MSG(checked_waitpid(invalid, status, 0),
                       ErrnoException,
                       "Failed to wait for a process.");
    }
    SUBCASE("terminated PID") {
      const auto pid = checked_fork();
      if (pid == 0) fast_exit(0);
      REQUIRE(pid > 0);

      int status = 0;
      CHECK(checked_waitpid(pid, status, 0) == pid);

      CHECK_THROWS_MSG(checked_waitpid(pid, status, 0),
                       ErrnoException,
                       "Failed to wait for a process.");
    }
  }
}

TEST_CASE("checked_kill") {
  SUBCASE("success") {
    // Note: Doctest installs its own signal handlers, so we reset to default
    //       here, otherwise it will report failure from the child process.
    const auto prev_handler = std::signal(SIGTERM, SIG_DFL);

    const auto pid = checked_fork();
    if (pid == 0) {
      std::this_thread::sleep_for(std::chrono::seconds{1});
      fast_exit(0);
    }

    REQUIRE(pid > 0);

    checked_kill(pid, SIGTERM);

    int status = 0;
    REQUIRE(checked_waitpid(pid, status, 0) == pid);

    CHECK(WIFSIGNALED(status));
    CHECK(WTERMSIG(status) == SIGTERM);

    // Restore Doctest's signal handler.
    CHECK(std::signal(SIGTERM, prev_handler) == SIG_DFL);
  }
  SUBCASE("failure") {
    SUBCASE("invalid PID") {
      CHECK_THROWS_MSG(checked_kill(invalid, SIGTERM),
                       ErrnoException,
                       "Failed to kill a process.");
    }
    SUBCASE("terminated PID") {
      const auto pid = checked_fork();
      if (pid == 0) fast_exit(0);
      REQUIRE(pid > 0);

      int status = 0;
      REQUIRE(checked_waitpid(pid, status, 0) == pid);

      CHECK_THROWS_MSG(checked_kill(pid, SIGTERM),
                       ErrnoException,
                       "Failed to kill a process.");
    }
    SUBCASE("invalid signal") {
      const auto pid = checked_fork();
      if (pid == 0) {
        std::this_thread::sleep_for(std::chrono::seconds{1});
        fast_exit(0);
      }

      REQUIRE(pid > 0);

      CHECK_THROWS_MSG(checked_kill(pid, invalid),
                       ErrnoException,
                       "Failed to kill a process.");

      int status = 0;
      REQUIRE(checked_waitpid(pid, status, 0) == pid);
    }
  }
}

TEST_CASE("checked_exec") {
  SUBCASE("success") {
    const auto pid = checked_fork();
    if (pid == 0) {
      checked_exec("/bin/sh", {"-c", "echo Hello, World!"});
      fast_exit(1);
    }

    REQUIRE(pid > 0);

    int status = 0;
    REQUIRE(checked_waitpid(pid, status, 0) == pid);

    CHECK(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 0);
  }
  SUBCASE("failure") {
    constexpr auto run_subcase = [](std::string_view file_name) {
      const auto pid = checked_fork();
      if (pid == 0) {
        try {
          checked_exec(file_name, {});
        } catch (const Exception&) {
          fast_exit(1);
        }
        fast_exit(0);
      }

      REQUIRE(pid > 0);

      int status = 0;
      REQUIRE(checked_waitpid(pid, status, 0) == pid);

      CHECK(WIFEXITED(status));
      CHECK(WEXITSTATUS(status) != 0);
    };
    SUBCASE("non-existent file") {
      run_subcase("/invalid/path");
    }
    SUBCASE("non-executable file") {
      run_subcase(__FILE__);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("FD") {
  SUBCASE("constructor") {
    SUBCASE("default") {
      const FD fd;
      CHECK(fd == -1);
    }
    SUBCASE("from raw file descriptor") {
      const auto raw_fd = checked_open(__FILE__, O_RDONLY);

      {
        const FD fd{raw_fd};
        CHECK(fd == raw_fd);
      }

      // Let's ensure that the file descriptor is closed.
      CHECK_THROWS_AS(checked_close(raw_fd), ErrnoException);
    }
  }
  SUBCASE("move") {
    SUBCASE("constructor") {
      const auto raw_fd = checked_open(__FILE__, O_RDONLY);

      FD fd1{raw_fd};
      REQUIRE(fd1 == raw_fd);

      const FD fd2{std::move(fd1)};
      CHECK(fd2 == raw_fd);
      CHECK(fd1 == -1);
    }
    SUBCASE("assignment") {
      SUBCASE("self") {
        const auto raw_fd = checked_open(__FILE__, O_RDONLY);

        FD fd{raw_fd};
        REQUIRE(fd == raw_fd);

        auto& fd_ref = fd;
        fd = std::move(fd_ref);
        CHECK(fd == raw_fd);
      }
      SUBCASE("other") {
        const auto raw_fd = checked_open(__FILE__, O_RDONLY);

        FD fd1{raw_fd};
        REQUIRE(fd1 == raw_fd);

        FD fd2;
        fd2 = std::move(fd1);
        CHECK(fd2 == raw_fd);
        CHECK(fd1 == -1);
      }
    }
  }
  SUBCASE("reset") {
    const auto raw_fd_1 = checked_open(__FILE__, O_RDONLY);
    const auto raw_fd_2 = checked_open(__FILE__, O_RDONLY);

    FD fd{raw_fd_1};
    REQUIRE(fd == raw_fd_1);

    fd.reset(raw_fd_2);
    CHECK(fd == raw_fd_2);
    CHECK_THROWS_AS(checked_close(raw_fd_1), ErrnoException);

    fd.reset();
    CHECK(fd == -1);
    CHECK_THROWS_AS(checked_close(raw_fd_2), ErrnoException);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Pipe") {
  SUBCASE("constructor") {
    SUBCASE("default") {
      Pipe pipe;
      CHECK(pipe.rend() == -1);
      CHECK(pipe.wend() == -1);
    }
    SUBCASE("from pipe") {
      const auto raw_pipe = checked_pipe();

      {
        Pipe pipe{raw_pipe};
        REQUIRE(pipe.rend() == raw_pipe[0]);
        REQUIRE(pipe.wend() == raw_pipe[1]);
      }

      // Let's ensure that the pipe is closed.
      CHECK_THROWS_AS(checked_close(raw_pipe[0]), ErrnoException);
      CHECK_THROWS_AS(checked_close(raw_pipe[1]), ErrnoException);
    }
  }
  SUBCASE("reset") {
    const auto raw_pipe_1 = checked_pipe();
    const auto raw_pipe_2 = checked_pipe();

    Pipe pipe{raw_pipe_1};
    REQUIRE(pipe.rend() == raw_pipe_1[0]);
    REQUIRE(pipe.wend() == raw_pipe_1[1]);

    pipe.reset(raw_pipe_2);
    CHECK(pipe.rend() == raw_pipe_2[0]);
    CHECK(pipe.wend() == raw_pipe_2[1]);
    CHECK_THROWS_AS(checked_close(raw_pipe_1[0]), ErrnoException);
    CHECK_THROWS_AS(checked_close(raw_pipe_1[1]), ErrnoException);

    pipe.reset();
    CHECK(pipe.rend() == -1);
    CHECK(pipe.wend() == -1);
    CHECK_THROWS_AS(checked_close(raw_pipe_2[0]), ErrnoException);
    CHECK_THROWS_AS(checked_close(raw_pipe_2[1]), ErrnoException);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Process") {
  SUBCASE("constructor") {
    SUBCASE("default") {
      const Process process;
      CHECK(process.pid() == -1);
      CHECK_FALSE(process.is_running());
    }
    SUBCASE("spawn") {
      SUBCASE("with callback") {
        pid_t pid = 0;

        {
          Process process;
          process.spawn_child([] { fast_exit(0); });

          CHECK(process.pid() > 0);
          CHECK(process.is_running());
          pid = process.pid();
        }

        // Let's ensure that the child process is killed.
        CHECK_THROWS_AS(checked_kill(pid, SIGTERM), ErrnoException);
      }
      SUBCASE("with file") {
        pid_t pid = 0;

        {
          Process process;
          process.spawn_child("/bin/sh", {"-c", "echo Hello, World!"});

          CHECK(process.pid() > 0);
          CHECK(process.is_running());
          pid = process.pid();
        }

        // Let's ensure that the child process is killed.
        CHECK_THROWS_AS(checked_kill(pid, SIGTERM), ErrnoException);
      }
    }
  }
  SUBCASE("move") {
    SUBCASE("constructor") {
      Process process_1;
      process_1.spawn_child([] { fast_exit(0); });

      REQUIRE(process_1.pid() > 0);
      const auto pid = process_1.pid();

      const Process process_2{std::move(process_1)};
      CHECK(process_2.pid() == pid);
      CHECK(process_1.pid() == -1);
    }
    SUBCASE("assignment") {
      SUBCASE("self") {
        Process process;
        process.spawn_child([] { fast_exit(0); });

        REQUIRE(process.pid() > 0);
        const auto pid = process.pid();

        auto& process_ref = process;
        process_ref = std::move(process);
        CHECK(process.pid() == pid);
      }
      SUBCASE("other") {
        SUBCASE("assign running to empty") {
          Process process_1;
          process_1.spawn_child([] { fast_exit(0); });

          const auto pid = process_1.pid();
          REQUIRE(pid > 0);

          Process process_2;
          REQUIRE(process_2.pid() == -1);

          process_2 = std::move(process_1);
          CHECK(process_2.pid() == pid);
          CHECK(process_1.pid() == -1);
        }
        SUBCASE("assign empty to running") {
          Process process_1;
          REQUIRE(process_1.pid() == -1);

          Process process_2;
          process_2.spawn_child([] { fast_exit(0); });

          const auto pid = process_2.pid();
          REQUIRE(pid > 0);

          process_2 = std::move(process_1);
          CHECK(process_1.pid() == -1);
          CHECK(process_2.pid() == -1);

          CHECK_THROWS_AS(checked_kill(pid, SIGTERM), ErrnoException);
        }
      }
    }
  }
  SUBCASE("wait_child") {
    Process process;
    process.spawn_child([] { fast_exit(0); });

    const auto pid = process.pid();
    REQUIRE(pid > 0);
    REQUIRE(process.is_running());

    process.wait_child();

    CHECK(process.pid() == -1);
    CHECK_FALSE(process.is_running());

    CHECK_THROWS_AS(checked_kill(pid, SIGTERM), ErrnoException);
  }
  SUBCASE("on_stdout/on_stderr/on_exit") {
    static constexpr auto exit_code = 13;
    const std::string stdout_message = "Hello, World!";
    const std::string stderr_message = "Warning: Danger!";

    std::string captured_stdout;
    std::string captured_stderr;
    int captured_exit_code = -1;
    int captured_signal = -1;

    Process process;

    process.on_stdout(
        [&captured_stdout](std::string_view data) { captured_stdout += data; });

    process.on_stderr(
        [&captured_stderr](std::string_view data) { captured_stderr += data; });

    process.on_exit([&captured_exit_code, &captured_signal](int code, int sig) {
      captured_exit_code = code;
      captured_signal = sig;
    });

    for (int n = 0; n < 3; ++n) {
      CAPTURE(std::format("iteration #{}", n));

      captured_stdout.clear();
      captured_stderr.clear();
      captured_exit_code = -1;
      captured_signal = -1;

      process.spawn_child([&stdout_message, &stderr_message] {
        checked_write(STDOUT_FILENO, stdout_message);
        checked_write(STDERR_FILENO, stderr_message);
        fast_exit(exit_code);
      });

      const auto pid = process.pid();
      CHECK(pid > 0);
      CHECK(process.is_running());

      process.wait_child();
      CHECK(process.pid() == -1);
      CHECK(!process.is_running());

      CHECK(captured_stdout == stdout_message);
      CHECK(captured_stderr == stderr_message);
      CHECK(captured_exit_code == exit_code);
      CHECK(captured_signal == 0);
    }
  }
  SUBCASE("kill_child") {
    const auto prev_handler = std::signal(SIGTERM, SIG_DFL);

    Process process;

    int captured_exit_code = 0;
    int captured_signal = -1;
    process.on_exit([&captured_exit_code, &captured_signal](int code, int sig) {
      captured_exit_code = code;
      captured_signal = sig;
    });

    for (int n = 0; n < 3; ++n) {
      CAPTURE(std::format("iteration #{}", n));

      captured_exit_code = 0;
      captured_signal = -1;

      process.spawn_child([] {
        std::this_thread::sleep_for(std::chrono::seconds{1});
        fast_exit(0);
      });

      process.kill_child(SIGTERM);
      process.wait_child();

      CHECK(captured_exit_code == -1);
      CHECK(captured_signal == SIGTERM);
    }

    CHECK(std::signal(SIGTERM, prev_handler) == SIG_DFL);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

// NOLINTEND(*-use-after-move,clang-analyzer-cplusplus.Move)
