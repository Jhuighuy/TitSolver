/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <memory>
#include <utility>
#include <vector>

#include <QApplication>
#include <QList>
#include <QMainWindow>
#include <QObject>
#include <QResource>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <QtCore/qtmetamacros.h>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/basic_types.hpp"
#include "tit/core/cmd.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/print.hpp"
#include "tit/core/sys/utils.hpp"
#include "tit/core/type.hpp"

#include "tit/data/storage.hpp"

static void init_qt_resources() {
  Q_INIT_RESOURCE(resources);
}

namespace tit::app {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class WebSocketServer : public QObject {
  Q_OBJECT
public:

  TIT_NOT_COPYABLE_OR_MOVABLE(WebSocketServer);

  explicit WebSocketServer(QObject* parent = nullptr)
      : QObject{parent}, server_{std::make_unique<QWebSocketServer>(
                             QString{"titback"},
                             QWebSocketServer::NonSecureMode,
                             this)} {
    if (server_->listen(QHostAddress::LocalHost,
                        get_env<uint16_t>("TIT_BACKEND_PORT", 18080))) {
      err("WS server is running on port {}.", server_->serverPort());
      connect(server_.get(),
              &QWebSocketServer::newConnection,
              this,
              &WebSocketServer::onNewConnection);
    }
  }

  ~WebSocketServer() override {
    server_->close();
  }

private slots:

  void onNewConnection() {
    auto* const socket = server_->nextPendingConnection();
    connect(socket,
            &QWebSocket::textMessageReceived,
            this,
            &WebSocketServer::onTextMessageReceived);
    connect(socket,
            &QWebSocket::disconnected,
            this,
            &WebSocketServer::onSocketDisconnected);
    clients_.emplace_back(socket);
  }

  void onSocketDisconnected() {
    auto* const client = qobject_cast<QWebSocket*>(sender());
    if (client == nullptr) return;
    clients_.removeAll(client);
    client->deleteLater();
  }

  void onTextMessageReceived(const QString& message) {
    auto* const client = qobject_cast<QWebSocket*>(sender());
    if (client == nullptr) return;

    const auto request = nlohmann::json::parse(message.toStdString());
    nlohmann::json response;
    response["status"] = "success";
    response["requestID"] = request["requestID"];
    const auto varyings = storage.last_series().last_time_step().varyings();
    for (const auto* var : {"r", "rho"}) {
      const auto r = varyings.find_array(var);
      TIT_ENSURE(r.has_value(), "Variable '{}' is not found.", var);
      std::vector<byte_t> r_data(r->size() * r->type().width());
      r->open_read()->read(r_data);
      response["result"][var] = std::span{
          safe_bit_ptr_cast<const double*>(std::as_const(r_data).data()),
          r_data.size() / sizeof(double),
      };
    }

    client->sendTextMessage(QString::fromStdString(response.dump()));
  }

private:

  std::unique_ptr<QWebSocketServer> server_;
  QList<QWebSocket*> clients_;
  data::DataStorage storage{"particles.ttdb", true};

}; // class WebSocketServer

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class WebWindow : public QMainWindow {
  Q_OBJECT
public:

  TIT_NOT_COPYABLE_OR_MOVABLE(WebWindow);

  WebWindow(QWidget* parent = nullptr)
      : QMainWindow{parent}, view_{std::make_unique<QWebEngineView>(this)} {
    view_->settings()->setAttribute(
        QWebEngineSettings::LocalContentCanAccessRemoteUrls,
        true);
    view_->settings()->setAttribute(
        QWebEngineSettings::LocalContentCanAccessFileUrls,
        true);
    view_->settings()->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);

    setCentralWidget(view_.get());

    connect(view_.get(),
            &QWebEngineView::titleChanged,
            this,
            &QMainWindow::setWindowTitle);

    const auto app_path = QCoreApplication::applicationDirPath();
    view_->load(QUrl{"qrc:/index.html"});
  }

  ~WebWindow() override = default;

private:

  std::unique_ptr<QWebEngineView> view_;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {
int app_main(CmdArgs args) {
  init_qt_resources();

  int argc = args.argc();
  char** argv = args.argv();
  QApplication app(argc, argv);

  const app::WebSocketServer server{&app};

  app::WebWindow window{};
  window.resize(1200, 800);
  window.show();

  return QApplication::exec();
}
} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::app

TIT_IMPLEMENT_MAIN(app::app_main)

#include "backend.moc"
