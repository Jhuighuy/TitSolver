/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <filesystem>

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QWebSocket>
#include <QWebSocketServer>
#include <qtmetamacros.h>

#include "tit/core/basic_types.hpp"
#include "tit/data/storage.hpp"

namespace tit::app {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// WebSocket server for the application.
class Server : public QObject {
  Q_OBJECT
public:

  /// Construct a server.
  explicit Server(uint16_t port,
                  const std::filesystem::path& exe_dir,
                  data::Storage storage,
                  QObject* parent = nullptr);

  /// Destruct a server.
  ~Server() override;

  /// Server is not copyable or movable.
  Server(Server&&) = delete;
  Server(const Server&) = delete;
  auto operator=(Server&&) -> Server& = delete;
  auto operator=(const Server&) -> Server& = delete;

private slots:

  // Handle new connections.
  void on_connection_();

  // Handle incoming messages.
  void on_message_(const QString& message);

private:

  // Handle incoming frame messages.
  void on_num_frames_message_(const QJsonObject& message,
                              const QJsonValue& request_id);
  void on_get_frame_message_(const QJsonObject& message,
                             const QJsonValue& request_id);

  // Handle incoming solver messages.
  void on_run_solver_message_(const QJsonObject& message,
                              const QJsonValue& request_id);
  void on_stop_solver_message_(const QJsonObject& message,
                               const QJsonValue& request_id);

  // Handle incoming export messages.
  void on_export_message_(const QJsonObject& message,
                          const QJsonValue& request_id);

  // Send a response to the client.
  void send_response_(const QJsonObject& response);
  void send_result_(const QJsonValue& request_id,
                    const QJsonValue& result = "",
                    bool repeat = false);
  void send_error_(const QJsonValue& request_id, const QJsonValue& result);

  QWebSocketServer* server_ = nullptr;
  QWebSocket* client_ = nullptr;
  data::Storage storage_;
  std::filesystem::path solver_path_;
  QProcess* solver_process_ = nullptr;

}; // class Server

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::app
