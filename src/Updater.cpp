/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file Updater.cpp
 *
 *  Copyright 2017 Sebastien Fourey
 *
 *  This file is part of G'MIC-Qt, a generic plug-in for raster graphics
 *  editors, offering hundreds of filters thanks to the underlying G'MIC
 *  image processing framework.
 *
 *  gmic_qt is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  gmic_qt is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gmic_qt.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "Updater.h"
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QTextStream>
#include <QUrl>
#include <iostream>
#include "Common.h"
#include "GmicStdlib.h"
#include "Logger.h"
#include "Misc.h"
#include "Settings.h"
#include "Utils.h"
#include "gmic.h"

namespace GmicQt
{
std::unique_ptr<Updater> Updater::_instance = std::unique_ptr<Updater>(nullptr);
OutputMessageMode Updater::_outputMessageMode = OutputMessageMode::Quiet;

const char * Updater::OfficialFilterSourceURL = "https://gmic.eu/update" GMIC_QT_XSTRINGIFY(gmic_version) ".gmic";

Updater::Updater(QObject * parent) : QObject(parent)
{
  _networkAccessManager = nullptr;
  _someNetworkUpdatesAchieved = false;
}

bool Updater::isCImgCompressed(const QByteArray & data)
{
  return data.startsWith("1 uint8 ");
}

Updater * Updater::getInstance()
{
  if (!_instance) {
    _instance = std::unique_ptr<Updater>(new Updater(nullptr));
  }
  return _instance.get();
}

Updater::~Updater() = default;

void Updater::startUpdate(int ageLimit, int timeout, bool useNetwork)
{
  TIMING;
  QStringList sources = GmicStdLib::substituteSourceVariables(Settings::filterSources());
  prependOfficialSourceIfRelevant(sources);
  SHOW(sources);
  _errorMessages.clear();
  _networkAccessManager = new QNetworkAccessManager(this);
  connect(_networkAccessManager, &QNetworkAccessManager::finished, this, &Updater::onNetworkReplyFinished);
  _someNetworkUpdatesAchieved = false;
  if (useNetwork) {
    QDateTime limit = QDateTime::currentDateTime().addSecs(-3600 * (qint64)ageLimit);
    for (const QString & str : sources) {
      if (str.startsWith("http://") || str.startsWith("https://")) {
        QString filename = localFilename(str);
        QFileInfo info(filename);
        if (!info.exists() || (info.lastModified() < limit)) {
          TRACE << "Downloading" << str << "to" << filename;
          QUrl url(str);
          QNetworkRequest request(url);
          request.setHeader(QNetworkRequest::UserAgentHeader, pluginFullName());
          // PRIVACY NOTICE (to be displayed in one of the "About" filters of the plugin
          //
          // PRIVACY NOTICE
          // This plugin may download up-to-date filter definitions from the gmic.eu server.
          // It is the case when first launched after a fresh installation, and periodically
          // with a frequency which can be set in the settings dialog.
          // The user should be aware that the following information may be retrieved
          // from the server logs: IP address of the client; date and time of the request;
          // as well as a short string, supplied through the HTTP protocol "User Agent" header
          // field, which describes the full plugin version as shown in the window title
          // (e.g. "G'MIC-Qt for GIMP 2.8 - Linux 64 bits - 2.2.1_pre#180301").
          //
          // Note that this information may solely be used for purely anonymous
          // statistical purposes.
          _pendingReplies.insert(_networkAccessManager->get(request));
        }
      }
    }
  }
  if (_pendingReplies.isEmpty()) {
    QTimer::singleShot(0, this, &Updater::onUpdateNotNecessary); // While GUI is Idle
    _networkAccessManager->deleteLater();
  } else {
    QTimer::singleShot(timeout * 1000, this, &Updater::cancelAllPendingDownloads);
  }
  TIMING;
}

QList<QString> Updater::errorMessages()
{
  return _errorMessages;
}

bool Updater::allDownloadsOk() const
{
  return _errorMessages.isEmpty();
}

void Updater::processReply(QNetworkReply * reply)
{
  QString url = reply->request().url().toString();
  if (!reply->bytesAvailable()) {
    return;
  }
  QByteArray array = reply->readAll();
  if (array.isNull()) {
    _errorMessages << QString(tr("Error downloading %1 (empty file?)")).arg(url);
    return;
  }
  if (isCImgCompressed(array)) {
    TRACE << QString("Decompressing reply from") << url;
    QByteArray tmp = cimgzDecompress(array);
    array = tmp;
  }
  if (array.isNull() || !array.contains("#@gui")) {
    _errorMessages << QString(tr("Could not read/decompress %1")).arg(url);
    return;
  }
  QString filename = localFilename(url);
  if (!safelyWrite(array, filename)) {
    _errorMessages << QString(tr("Error writing file %1")).arg(filename);
    return;
  }
  _someNetworkUpdatesAchieved = true;
}

void Updater::onNetworkReplyFinished(QNetworkReply * reply)
{
  TIMING;
  QNetworkReply::NetworkError error = reply->error();
  if (error == QNetworkReply::NoError) {
    processReply(reply);
  } else {
    QString str;
    QDebug d(&str);
    d << error;
    str = str.trimmed();
    _errorMessages << tr("Error downloading %1<br/>Error %2: %3").arg(reply->request().url().toString()).arg(static_cast<int>(error)).arg(str);
    Logger::error("Update failed");
    Logger::note(QString("Error string: %1").arg(reply->errorString()));
    Logger::note("******* Full reply contents ******\n");
    Logger::note(reply->readAll());
    Logger::note(QString("******** HTTP Status: %1").arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()));
    // We either create an empty local file or 'touch' the existing one to prevent a systematic update on next startups
    // Instead, usual delay will occur before next try
    touchFile(localFilename(reply->url().toString()));
  }
  _pendingReplies.remove(reply);
  if (_pendingReplies.isEmpty()) {
    if (_errorMessages.isEmpty()) {
      emit updateIsDone((int)UpdateStatus::Successful);
    } else {
      emit updateIsDone((int)UpdateStatus::SomeFailed);
    }
    _networkAccessManager->deleteLater();
    _networkAccessManager = nullptr;
  }
  reply->deleteLater();
}

void Updater::notifyAllDownloadsOK()
{
  _errorMessages.clear();
  emit updateIsDone((int)UpdateStatus::Successful);
}

void Updater::cancelAllPendingDownloads()
{
  TIMING;
  // Make a copy because aborting will call onNetworkReplyFinished, and
  // thus modify the _pendingReplies set.
  QSet<QNetworkReply *> replies = _pendingReplies;
  for (QNetworkReply * reply : replies) {
    _errorMessages << QString(tr("Download timeout: %1")).arg(reply->request().url().toString());
    reply->abort();
  }
}

void Updater::onUpdateNotNecessary()
{
  emit updateIsDone((int)UpdateStatus::NotNecessary);
}

QByteArray Updater::cimgzDecompress(const QByteArray & array)
{
  try {
    gmic_library::gmic_image<char> buffer(array.constData(), array.size(), 1, 1, 1, true);
    gmic_library::gmic_list<char> list = gmic_library::gmic_list<char>::get_unserialize(buffer);
    if (list.size() == 1) {
      return {list[0].data(), int(list[0].size())};
    }
  } catch (...) {
    Logger::warning("Updater::cimgzDecompress(): Error decompressing data");
  }
  return {};
}

QByteArray Updater::cimgzDecompressFile(const QString & filename)
{
  gmic_library::gmic_image<unsigned char> buffer;
  try {
    buffer.load_cimg(filename.toLocal8Bit().constData());
  } catch (...) {
    Logger::warning("Updater::cimgzDecompressFile(): gmic_image<>::load_cimg error for file " + filename);
    return QByteArray();
  }
  return QByteArray((char *)buffer.data(), (int)buffer.size());
}

QString Updater::localFilename(QString url)
{
  if (url.startsWith("http://") || url.startsWith("https://")) {
    QUrl u(url);
    return QString("%1%2").arg(gmicConfigPath(true)).arg(u.fileName());
  }
  return url;
}

bool Updater::appendLocalGmicFile(QByteArray & array, QString filename) const
{
  if (QFileInfo(filename).exists()) {
    return false;
  }
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    Logger::error("Error opening file: " + filename);
    return false;
  }
  QByteArray fileData;
  if (isCImgCompressed(file.peek(10))) {
    file.close();
    TRACE << "Appending compressed file:" << filename;
    fileData = cimgzDecompressFile(filename);
    if (!fileData.size()) {
      return false;
    }
  } else {
    TRACE << "Appending:" << filename;
    fileData = file.readAll();
  }
  array.append(fileData);
  array.append('\n');
  return true;
}

void Updater::prependOfficialSourceIfRelevant(QStringList & list)
{
  if (Settings::officialFilterSource() == SourcesWidget::OfficialFilters::EnabledWithUpdates) {
    list.push_front(QString::fromUtf8(OfficialFilterSourceURL));
  }
}

QByteArray Updater::buildFullStdlib() const
{
  QByteArray result;
  const QByteArray ToTopLevelSeparator = QString("#@gui %1\n").arg(QString("_").repeated(80)).toUtf8();

  QStringList sources = GmicStdLib::substituteSourceVariables(Settings::filterSources());

  switch (Settings::officialFilterSource()) {
  case SourcesWidget::OfficialFilters::Disabled:
    // No stdlib included
    break;
  case SourcesWidget::OfficialFilters::EnabledWithoutUpdates:
    appendBuiltinGmicStdlib(result);
    result.append(ToTopLevelSeparator);
    break;
  case SourcesWidget::OfficialFilters::EnabledWithUpdates:
    if (!appendLocalGmicFile(result, localFilename(QString::fromUtf8(OfficialFilterSourceURL)))) {
      // Fallback on builtin stdlib
      appendBuiltinGmicStdlib(result);
    }
    result.append(ToTopLevelSeparator);
    break;
  }

  for (const QString & source : sources) {
    QString filename = localFilename(source);
    if (appendLocalGmicFile(result, filename)) {
      result.append(ToTopLevelSeparator);
    }
  }
  return result;
}

bool Updater::someNetworkUpdateAchieved() const
{
  return _someNetworkUpdatesAchieved;
}

void Updater::setOutputMessageMode(OutputMessageMode mode)
{
  _outputMessageMode = mode;
}

void Updater::appendBuiltinGmicStdlib(QByteArray & array) const
{
  gmic_image<char> stdlib_h = gmic::decompress_stdlib();
  if (!stdlib_h.size() || (stdlib_h.size() == 1)) {
    Logger::error("Could not decompress gmic builtin stdlib");
    return;
  }
  QByteArray tmp(stdlib_h, int(stdlib_h.size() - 1));
  array.append(tmp);
  array.append('\n');
}

} // namespace GmicQt
