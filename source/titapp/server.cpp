/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <exception>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <ostream>
#include <ranges>
#include <utility>

#include <QCoreApplication>
#include <QFileDialog>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QWebSocket>
#include <QWebSocketProtocol>
#include <QWebSocketServer>
#include <QtAlgorithms>
#include <QtTypes>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/print.hpp"
#include "tit/core/type.hpp"
#include "tit/data/hdf5.hpp"
#include "tit/data/storage.hpp"
#include "titapp/server.hpp"

namespace tit::app {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Server::Server(uint16_t port,
               const std::filesystem::path& exe_dir,
               data::Storage storage,
               QObject* parent)
    : QObject{parent}, storage_{std::move(storage)},
      solver_path_{exe_dir / "titwcsph"} {
  server_ = new QWebSocketServer{QCoreApplication::applicationName(),
                                 QWebSocketServer::NonSecureMode,
                                 this};
  if (server_->listen(QHostAddress::Any, port)) {
    log("WebSocket server running on port {}.", port);
    std::flush(std::cout);
    connect(server_,
            &QWebSocketServer::newConnection,
            this,
            &Server::on_connection_);
  } else {
    TIT_THROW("Failed to start WebSocket server: {}.",
              server_->errorString().toLatin1().data());
  }
}

Server::~Server() {
  server_->close();

  if (solver_process_ != nullptr &&
      solver_process_->state() != QProcess::NotRunning) {
    solver_process_->kill();
    solver_process_->waitForFinished();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Server::on_connection_() {
  auto* const next_client = server_->nextPendingConnection();
  if (client_ != nullptr) {
    next_client->close(QWebSocketProtocol::CloseCodePolicyViolated);
    next_client->deleteLater();
    return;
  }

  client_ = next_client;
  client_->setParent(this);
  log("Client connected: {}.",
      client_->peerAddress().toString().toLatin1().data());

  connect(client_,
          &QWebSocket::textMessageReceived,
          this,
          &Server::on_message_);

  connect(client_, &QWebSocket::disconnected, this, [this] {
    if (client_ == nullptr) return;
    client_->deleteLater();
    client_ = nullptr;
    log("Client disconnected.");
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Server::on_message_(const QString& message) {
  QJsonParseError parse_error;
  const auto doc = QJsonDocument::fromJson(message.toUtf8(), &parse_error);
  if (parse_error.error != QJsonParseError::NoError) {
    err("JSON parse error: {}.", parse_error.errorString().toLatin1().data());
    return;
  }

  const auto root = doc.object();
  const auto request_id = root["requestID"];
  const auto params = root["message"].toObject();
  const auto type = params["type"].toString();

  try {
    if (type == "num-frames") {
      on_num_frames_message_(params, request_id);
    } else if (type == "frame") {
      on_get_frame_message_(params, request_id);
    } else if (type == "run") {
      on_run_solver_message_(params, request_id);
    } else if (type == "stop") {
      on_stop_solver_message_(params, request_id);
    } else if (type == "export") {
      on_export_message_(params, request_id);
    } else {
      send_error_(request_id, QString{"Unknown message type: '%1'."}.arg(type));
    }
  } catch (const std::exception& e) {
    send_error_(request_id, QString::fromUtf8(e.what()));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Server::on_num_frames_message_(const QJsonObject& /*message*/,
                                    const QJsonValue& request_id) {
  send_result_(request_id,
               static_cast<qint64>(storage_.last_series().num_frames()));
}

void Server::on_get_frame_message_(const QJsonObject& message,
                                   const QJsonValue& request_id) {
  auto frames = storage_.last_series().frames();
  const auto frame_index = static_cast<ssize_t>(message["index"].toInt());
  TIT_ENSURE(frame_index >= 0, "Frame index cannot be negative!");
  auto frame_iter = std::ranges::begin(frames);
  const auto frames_end = std::ranges::end(frames);
  std::ranges::advance(frame_iter, frame_index, frames_end);
  TIT_ENSURE(frame_iter != frames_end, "Frame index out of bounds!");

  QJsonObject result;
  for (const auto& array : (*frame_iter).arrays()) {
    const auto bytes = array.read();
    const QByteArray bytes_array{safe_bit_ptr_cast<const char*>(bytes.data()),
                                 static_cast<int64_t>(bytes.size())};

    QJsonObject object;
    object["kind"] = QString::fromStdString(array.type().kind().name());
    object["data"] = QString::fromLatin1(bytes_array.toBase64());
    result[QString::fromUtf8(array.name())] = object;
  }

  send_result_(request_id, result);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Server::on_run_solver_message_(const QJsonObject& /*message*/,
                                    const QJsonValue& request_id) {
  if (solver_process_ != nullptr &&
      solver_process_->state() != QProcess::NotRunning) {
    send_error_(request_id, "Solver is already running.");
    return;
  }

  if (solver_process_ != nullptr) solver_process_->deleteLater();
  solver_process_ = new QProcess{this};

  connect(solver_process_,
          &QProcess::readyReadStandardOutput,
          this,
          [this, request_id] {
            QJsonObject result;
            result["kind"] = "stdout";
            result["data"] =
                QString::fromUtf8(solver_process_->readAllStandardOutput());
            send_result_(request_id, result, /*repeat=*/true);
          });

  connect(solver_process_,
          &QProcess::readyReadStandardError,
          this,
          [this, request_id] {
            QJsonObject result;
            result["kind"] = "stderr";
            result["data"] =
                QString::fromUtf8(solver_process_->readAllStandardError());
            send_result_(request_id, result, /*repeat=*/true);
          });

  connect(solver_process_,
          &QProcess::finished,
          this,
          [this, request_id](int exit_code, QProcess::ExitStatus exit_status) {
            QJsonObject result;
            result["kind"] = "exit";
            result["code"] = exit_code;
            result["signal"] = (exit_status == QProcess::CrashExit) ? 1 : 0;
            send_result_(request_id, result);
          });

  solver_process_->start(QString::fromStdString(solver_path_));
}

void Server::on_stop_solver_message_(const QJsonObject& /*message*/,
                                     const QJsonValue& request_id) {
  if (solver_process_ == nullptr ||
      solver_process_->state() == QProcess::NotRunning) {
    send_error_(request_id, "Solver is not running.");
    return;
  }

  solver_process_->terminate();

  send_result_(request_id);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Server::on_export_message_(const QJsonObject& /*message*/,
                                const QJsonValue& request_id) {
  /// @todo This will fail in headless mode. We should restore the old behavior
  ///       and export to a default location in that case.
  const auto dir_path = QFileDialog::getExistingDirectory();
  if (dir_path.isEmpty()) {
    send_result_(request_id);
    return;
  }

  const std::filesystem::path out_dir{dir_path.toUtf8().data()};
  std::filesystem::create_directories(out_dir);
  data::export_hdf5(out_dir, storage_.last_series());
  send_result_(request_id);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Server::send_response_(const QJsonObject& response) {
  if (client_ == nullptr) return;
  const QJsonDocument doc{response};
  client_->sendTextMessage(
      QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void Server::send_result_(const QJsonValue& request_id,
                          const QJsonValue& result,
                          bool repeat) {
  QJsonObject response;
  response["requestID"] = request_id;
  response["status"] = "success";
  response["result"] = result;
  if (repeat) response["repeat"] = true;
  send_response_(response);
}

void Server::send_error_(const QJsonValue& request_id,
                         const QJsonValue& result) {
  QJsonObject response;
  response["requestID"] = request_id;
  response["status"] = "error";
  response["result"] = result;
  send_response_(response);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::app
