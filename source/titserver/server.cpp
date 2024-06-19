#include <iostream>
#include <ostream>

#include <httplib.h>

#include "tit/core/main_func.hpp"

namespace {

auto serv_main(int /*argc*/, char** /*argv*/) -> int {
  httplib::Server server;

  server.Get("/hello", [](const httplib::Request&, httplib::Response& res) {
    std::cout << "GET /hello\n";
    std::flush(std::cout);
    res.set_content("Hello from C++!", "text/plain");
  });

  std::cout << "Server is running on http://localhost:18080\n";
  std::flush(std::cout);
  server.listen("localhost", 18080);

  return 0;
}

} // namespace

auto main(int argc, char** argv) -> int {
  using namespace tit;
  run_main(argc, argv, &serv_main);
}
