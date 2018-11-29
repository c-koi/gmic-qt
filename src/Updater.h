/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file Updater.h
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
#ifndef _GMIC_QT_UPDATER_H_
#define _GMIC_QT_UPDATER_H_

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QSet>
#include <QSettings>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>
#include <memory>

#include "gmic_qt.h"
class Updater : public QObject {
  Q_OBJECT

public:
  enum UpdateStatus
  {
    UpdateSuccessful,
    SomeUpdatesFailed,
    UpdateNotNecessary
  };

  static Updater * getInstance();
  static void setOutputMessageMode(GmicQt::OutputMessageMode mode);
  ~Updater();

  /**
   * @brief Launch download of files that are either not present locally, or
   *        older than the given age limit (in hours). To force download
   *        of all the sources, set the age limit to zero.
   *
   * @param ageLimit Delay bewteen 2 network updates in hours
   * @param timeout in seconds before aborting dowloads
   * @param useNetwork Enable internet access
   */
  void startUpdate(int ageLimit, int timeout, bool useNetwork);

  QList<QString> errorMessages();
  QList<QString> remotesThatNeedUpdate(int ageLimit) const;
  bool someUpdatesNeeded(int ageLimit) const;
  bool allDownloadsOk() const;
  QList<QString> sources() const;
  QByteArray buildFullStdlib() const;

  bool someNetworkUpdateAchieved() const;

  void updateSources(bool useNetwork);

signals:
  void updateIsDone(int status);

public slots:
  void onNetworkReplyFinished(QNetworkReply *);
  void notifyAllDowloadsOK();
  void cancelAllPendingDownloads();
  void onUpdateNotNecessary();

protected:
  void processReply(QNetworkReply * reply);

private:
  static QString localFilename(QString url);
  bool isStdlib(const QString & source) const;

  explicit Updater(QObject * parent);
  static QByteArray cimgzDecompress(const QByteArray & array);
  static QByteArray cimgzDecompressFile(const QString & filename);
  static std::unique_ptr<Updater> _instance;
  static GmicQt::OutputMessageMode _outputMessageMode;

  QNetworkAccessManager * _networkAccessManager;
  QList<QString> _sources;
  QMap<QString, bool> _sourceIsStdLib;
  QSet<QNetworkReply *> _pendingReplies;
  QList<QString> _errorMessages;
  bool _someNetworkUpdatesAchieved;
};

#endif // _GMIC_QT_UPDATER_H_
