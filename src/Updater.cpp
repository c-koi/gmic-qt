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
#include <QDebug>
#include <QNetworkRequest>
#include <QUrl>
#include <iostream>
#include "Common.h"
#include "GmicStdlib.h"
#include "Logger.h"
#include "Utils.h"
#include "gmic.h"

std::unique_ptr<Updater> Updater::_instance = std::unique_ptr<Updater>(nullptr);
GmicQt::OutputMessageMode Updater::_outputMessageMode = GmicQt::Quiet;

Updater::Updater(QObject * parent) : QObject(parent)
{
  _networkAccessManager = 0;
  _someNetworkUpdatesAchieved = false;
}

Updater * Updater::getInstance()
{
  if (!_instance.get()) {
    _instance = std::unique_ptr<Updater>(new Updater(nullptr));
  }
  return _instance.get();
}

Updater::~Updater() {}

void Updater::updateSources(bool useNetwork)
{
  _sources.clear();
  _sourceIsStdLib.clear();
  // Build sources map
  QString prefix;
  if (_outputMessageMode >= GmicQt::DebugConsole) {
    prefix = "debug ";
  } else if (_outputMessageMode >= GmicQt::VerboseLayerName) {
    prefix = "v -99 ";
  } else {
    prefix = "v - ";
  }
  cimg_library::CImgList<gmic_pixel_type> gptSources;
  cimg_library::CImgList<char> names;
  QString command = QString("%1gui_filter_sources %2").arg(prefix).arg(useNetwork);
  try {
    gmic(command.toLocal8Bit().constData(), gptSources, names, 0, true);
  } catch (...) {
    gptSources.assign();
  }
  cimg_library::CImgList<char> sources;
  gptSources.move_to(sources);
  cimglist_for(sources, l)
  {
    cimg_library::CImg<char> & str = sources[l];
    str.unroll('x');
    bool isStdlib = (str.back() == 1);
    if (isStdlib) {
      str.back() = 0;
    } else {
      str.columns(0, str.width());
    }
    QString source = QString::fromLocal8Bit(str);
    _sources << source;
    _sourceIsStdLib[source] = isStdlib;
  }

  // NOTE : For testing purpose
  //  _sources.clear();
  //  _sourceIsStdLib.clear();
  //  _sources.push_back("http://localhost:2222/update220.gmic");
  //  _sourceIsStdLib["http://localhost:2222/update220.gmic"] = true;

  SHOW(_sources);
}

void Updater::startUpdate(int ageLimit, int timeout, bool useNetwork)
{
  TIMING;
  updateSources(useNetwork);
  _errorMessages.clear();
  _networkAccessManager = new QNetworkAccessManager(this);
  connect(_networkAccessManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(onNetworkReplyFinished(QNetworkReply *)));
  _someNetworkUpdatesAchieved = false;
  if (useNetwork) {
    QDateTime limit = QDateTime::currentDateTime().addSecs(-3600 * (qint64)ageLimit);
    for (QString str : _sources) {
      if (str.startsWith("http://") || str.startsWith("https://")) {
        QString filename = localFilename(str);
        QFileInfo info(filename);
        if (!info.exists() || info.lastModified() < limit) {
          TRACE << "Downloading" << str << "to" << filename;
          QUrl url(str);
          QNetworkRequest request(url);
          request.setHeader(QNetworkRequest::UserAgentHeader, GmicQt::pluginFullName());
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
    QTimer::singleShot(0, this, SLOT(onUpdateNotNecessary())); // While GUI is Idle
    _networkAccessManager->deleteLater();
  } else {
    QTimer::singleShot(timeout * 1000, this, SLOT(cancelAllPendingDownloads()));
  }
  TIMING;
}

QList<QString> Updater::remotesThatNeedUpdate(int ageLimit) const
{
  QDateTime limit = QDateTime::currentDateTime().addSecs(-3600 * ageLimit);
  QList<QString> list;
  for (QString str : _sources) {
    if (str.startsWith("http://") || str.startsWith("https://")) {
      QString filename = localFilename(str);
      QFileInfo info(filename);
      if (!info.exists() || info.lastModified() < limit) {
        list << str;
      }
    }
  }
  return list;
}

bool Updater::someUpdatesNeeded(int ageLimit) const
{
  QDateTime limit = QDateTime::currentDateTime().addSecs(-3600 * ageLimit);
  for (QString str : _sources) {
    if (str.startsWith("http://") || str.startsWith("https://")) {
      QString filename = localFilename(str);
      QFileInfo info(filename);
      if (!info.exists() || info.lastModified() < limit) {
        return true;
      }
    }
  }
  return false;
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
  if (!array.startsWith("#@gmic")) {
    TRACE << "Decompressing reply from" << url;
    QByteArray tmp = cimgzDecompress(array);
    array = tmp;
  }
  if (array.isNull() || !array.startsWith("#@gmic")) {
    _errorMessages << QString(tr("Could not read/decompress %1")).arg(url);
    return;
  }
  QString filename = localFilename(url);
  QFile file(filename);
  if (!file.open(QFile::WriteOnly)) {
    _errorMessages << QString(tr("Error creating file %1")).arg(filename);
    return;
  }
  if (file.write(array) != array.size()) {
    _errorMessages << QString(tr("Error writing file %1")).arg(filename);
  } else {
    _someNetworkUpdatesAchieved = true;
  }
}

void Updater::onNetworkReplyFinished(QNetworkReply * reply)
{
  TIMING;
  if (reply->error() == QNetworkReply::NoError) {
    processReply(reply);
  } else {
    _errorMessages << QString(tr("Error downloading %1")).arg(reply->request().url().toString());
  }
  _pendingReplies.remove(reply);
  if (_pendingReplies.isEmpty()) {
    if (_errorMessages.isEmpty()) {
      emit updateIsDone(UpdateSuccessful);
    } else {
      emit updateIsDone(SomeUpdatesFailed);
    }
    _networkAccessManager->deleteLater();
    _networkAccessManager = 0;
  }
  reply->deleteLater();
}

void Updater::notifyAllDowloadsOK()
{
  _errorMessages.clear();
  emit updateIsDone(UpdateSuccessful);
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
  emit updateIsDone(UpdateNotNecessary);
}

QByteArray Updater::cimgzDecompress(QByteArray array)
{
  QTemporaryFile tmpZ(QDir::tempPath() + QDir::separator() + "gmic_qt_update_XXXXXX_cimgz");
  if (!tmpZ.open()) {
    qWarning() << "Updater::cimgzDecompress(): Error creating" << tmpZ.fileName();
    return QByteArray();
  }
  tmpZ.write(array);
  tmpZ.flush();
  tmpZ.close();
  std::FILE * file = fopen(tmpZ.fileName().toLocal8Bit().constData(), "rb");
  if (!file) {
    qWarning() << "Updater::cimgzDecompress(): Error opening" << tmpZ.fileName();
    return QByteArray();
  }
  cimg_library::CImg<unsigned char> buffer;
  try {
    buffer.load_cimg(file);
  } catch (...) {
    qWarning() << "Updater::cimgzDecompress(): CImg<>::load_cimg error for file " << tmpZ.fileName();
    return QByteArray();
  }
  return QByteArray((char *)buffer.data(), buffer.size());
}

QByteArray Updater::cimgzDecompressFile(QString filename)
{
  cimg_library::CImg<unsigned char> buffer;
  try {
    buffer.load_cimg(filename.toLocal8Bit().constData());
  } catch (...) {
    qWarning() << "Updater::cimgzDecompressFile(): CImg<>::load_cimg error for file " << filename;
    return QByteArray();
  }
  return QByteArray((char *)buffer.data(), buffer.size());
}

QString Updater::localFilename(QString url)
{
  if (url.startsWith("http://") || url.startsWith("https://")) {
    QUrl u(url);
    return QString("%1%2").arg(GmicQt::path_rc(true)).arg(u.fileName());
  } else {
    return url;
  }
}

bool Updater::isStdlib(const QString & source) const
{
  QMap<QString, bool>::const_iterator it = _sourceIsStdLib.find(source);
  if (it != _sourceIsStdLib.end()) {
    return it.value();
  }
  return false;
}

QList<QString> Updater::sources() const
{
  return _sources;
}

QByteArray Updater::buildFullStdlib() const
{
  QByteArray result;
  if (_sources.isEmpty()) {
    gmic_image<char> stdlib_h = gmic::decompress_stdlib();
    QByteArray tmp = QByteArray::fromRawData(stdlib_h, stdlib_h.size());
    tmp[tmp.size() - 1] = '\n';
    result.append(tmp);
    return result;
  }
  for (QString source : _sources) {
    QString filename = localFilename(source);
    QFile file(filename);
    if (file.open(QFile::ReadOnly)) {
      QByteArray array;
      if (isStdlib(source) && !file.peek(10).startsWith("#@gmic")) {
        // Try to uncompress
        file.close();
        TRACE << "Appending compressed file:" << filename;
        array = cimgzDecompressFile(filename);
        if (array.size() && !array.startsWith("#@gmic")) {
          array.clear();
        }
        if (!array.size()) {
          gmic_image<char> stdlib_h = gmic::decompress_stdlib();
          QByteArray tmp = QByteArray::fromRawData(stdlib_h, stdlib_h.size());
          tmp[tmp.size() - 1] = '\n';
          array.append(tmp);
        }
      } else {
        TRACE << "Appending:" << filename;
        array = file.readAll();
      }
      result.append(array);
      result.append('\n');
    } else if (isStdlib(source)) {
      gmic_image<char> stdlib_h = gmic::decompress_stdlib();
      QByteArray tmp = QByteArray::fromRawData(stdlib_h, stdlib_h.size());
      tmp[tmp.size() - 1] = '\n';
      result.append(tmp);
    }
    result.append(QString("#@gui ") + QString("_").repeated(80) + QString("\n"));
  }
  return result;
}

bool Updater::someNetworkUpdateAchieved() const
{
  return _someNetworkUpdatesAchieved;
}

void Updater::setOutputMessageMode(GmicQt::OutputMessageMode mode)
{
  _outputMessageMode = mode;
}
