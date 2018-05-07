/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterThread.h
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
#ifndef _GMIC_QT__FILTERTHREAD_H_
#define _GMIC_QT__FILTERTHREAD_H_

#include <QThread>
#include <QTime>

class ImageSource;
class QMutex;
class QImage;
class QSemaphore;
#include <QApplication>
#include <QColor>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QMutex>
#include <QPainter>
#include <QTime>

#include "Common.h"
#include "Host/host.h"
#include "gmic_qt.h"

namespace cimg_library
{
template <typename T> struct CImgList;
}

class FilterThread : public QThread {
  Q_OBJECT

public:
  FilterThread(QObject * parent, const QString & name, const QString & command, const QString & arguments, const QString & environment, GmicQt::OutputMessageMode mode);

  virtual ~FilterThread();
  void setArguments(const QString &);
  void setInputImages(const cimg_library::CImgList<float> & list);
  void setImageNames(const cimg_library::CImgList<char> & imageNames);
  void swapImages(cimg_library::CImgList<float> & images);
  const cimg_library::CImgList<float> & images() const;
  const cimg_library::CImgList<char> & imageNames() const;
  QStringList gmicStatus() const;
  QString errorMessage() const;
  bool failed() const;
  bool aborted() const;
  int duration() const;
  float progress() const;
  QString name() const;
  QString fullCommand() const;
  void setLogSuffix(const QString & text);

public slots:
  void abortGmic();

signals:
  void done();

protected:
  void run() override;

private:
  QString _command;
  QString _arguments;
  QString _environment;
  cimg_library::CImgList<float> * _images;
  cimg_library::CImgList<char> * _imageNames;
  bool _gmicAbort;
  bool _failed;
  QString _gmicStatus;
  float _gmicProgress;
  QString _errorMessage;
  QString _name;
  QString _logSuffix;
  GmicQt::OutputMessageMode _messageMode;
  QTime _startTime;
};

#endif // _GMIC_QT__FILTERTHREAD_H_
