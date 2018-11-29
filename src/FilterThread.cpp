/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterThread.cpp
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
#include "FilterThread.h"
#include <QDebug>
#include <iostream>
#include "GmicStdlib.h"
#include "ImageConverter.h"
#include "Logger.h"
#include "Utils.h"
#include "gmic.h"
using namespace cimg_library;

FilterThread::FilterThread(QObject * parent, const QString & name, const QString & command, const QString & arguments, const QString & environment, GmicQt::OutputMessageMode mode)
    : QThread(parent), _command(command), _arguments(arguments), _environment(environment), _images(new cimg_library::CImgList<float>), _imageNames(new cimg_library::CImgList<char>), _name(name),
      _messageMode(mode)
{
  _gmicAbort = false;
  _failed = false;
  _gmicProgress = 0.0f;
  // ENTERING;
#ifdef _IS_MACOS_
  setStackSize(8 * 1024 * 1024);
#endif
}

FilterThread::~FilterThread()
{
  delete _images;
  delete _imageNames;
}

void FilterThread::setArguments(const QString & str)
{
  _arguments = str;
}

void FilterThread::setImageNames(const cimg_library::CImgList<char> & imageNames)
{
  *_imageNames = imageNames;
}

void FilterThread::swapImages(cimg_library::CImgList<float> & images)
{
  _images->swap(images);
}

void FilterThread::setInputImages(const cimg_library::CImgList<float> & list)
{
  *_images = list;
}

const cimg_library::CImgList<float> & FilterThread::images() const
{
  return *_images;
}

const cimg_library::CImgList<char> & FilterThread::imageNames() const
{
  return *_imageNames;
}

QStringList FilterThread::gmicStatus() const
{
  if (!_gmicStatus.startsWith(QChar(24)) || !_gmicStatus.endsWith(QChar(25))) {
    return QStringList();
  }
  QList<QString> list = _gmicStatus.split(QString("%1%2").arg(QChar(25)).arg(QChar(24)));
  if (!list.isEmpty()) {
    list[0].remove(0, 1);
    list.back().chop(1);
    QList<QString>::iterator it = list.begin();
    while (it != list.end()) {
      QByteArray array = it->toLocal8Bit();
      gmic::strreplace_fw(array.data());
      *it++ = array;
    }
  }
  return list;
}

QString FilterThread::errorMessage() const
{
  return _errorMessage;
}

bool FilterThread::failed() const
{
  return _failed;
}

bool FilterThread::aborted() const
{
  return _gmicAbort;
}

int FilterThread::duration() const
{
  return _startTime.elapsed();
}

float FilterThread::progress() const
{
  return _gmicProgress;
}

QString FilterThread::name() const
{
  return _name;
}

QString FilterThread::fullCommand() const
{
  return QString("%1 %2").arg(_command).arg(_arguments);
}

void FilterThread::setLogSuffix(const QString & text)
{
  _logSuffix = text;
}

void FilterThread::abortGmic()
{
  _gmicAbort = true;
}

void FilterThread::run()
{
  _startTime.start();
  _errorMessage.clear();
  _failed = false;
  QString fullCommandLine;
  try {
    fullCommandLine = QString::fromLocal8Bit(GmicQt::commandFromOutputMessageMode(_messageMode));
    fullCommandLine += QString(" %1 %2").arg(_command).arg(_arguments);
    _gmicAbort = false;
    _gmicProgress = -1;
    if (_messageMode > GmicQt::Quiet) {
      Logger::log(QString("\n[%1]%2 %3\n").arg(GmicQt::pluginCodeName()).arg(_logSuffix).arg(fullCommandLine));
    }
    gmic gmicInstance(_environment.isEmpty() ? nullptr : QString("v - %1").arg(_environment).toLocal8Bit().constData(), GmicStdLib::Array.constData(), true);
    gmicInstance.set_variable("_host", GmicQt::HostApplicationShortname, '=');
    gmicInstance.set_variable("_tk", "qt", '=');
    gmicInstance.run(fullCommandLine.toLocal8Bit().constData(), *_images, *_imageNames, &_gmicProgress, &_gmicAbort);
    _gmicStatus = gmicInstance.status;
  } catch (gmic_exception & e) {
    _images->assign();
    _imageNames->assign();
    const char * message = e.what();
    _errorMessage = message;
    if (_messageMode > GmicQt::Quiet) {
      Logger::log(QString("\n[%1]./error/ When running command '%2', this error occured:\n%3\n").arg(GmicQt::pluginCodeName()).arg(fullCommandLine).arg(message));
    }
    _failed = true;
  }
}
