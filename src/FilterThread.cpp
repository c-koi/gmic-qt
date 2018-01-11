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
#include "GmicStdlibParser.h"
#include "ImageConverter.h"
#include "gmic.h"
using namespace cimg_library;

FilterThread::FilterThread(QObject * parent, const QString & name, const QString & command, const QString & arguments, const QString & environment, GmicQt::OutputMessageMode mode)
    : QThread(parent), _command(command), _arguments(arguments), _environment(environment), _images(new cimg_library::CImgList<float>), _imageNames(new cimg_library::CImgList<char>), _name(name),
      _messageMode(mode)
{
  ENTERING;
#ifdef _IS_MACOS_
  setStackSize(8 * 1024 * 1024);
#endif
}

FilterThread::~FilterThread()
{
  ENTERING;
  delete _images;
  delete _imageNames;
}

void FilterThread::setArguments(const QString & str)
{
  _arguments = str;
}

void FilterThread::setInputImages(const cimg_library::CImgList<float> & list, const cimg_library::CImgList<char> & imageNames)
{
  *_images = list;
  *_imageNames = imageNames;
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
  if (list.size()) {
    list[0].remove(0, 1);
    list.back().chop(1);
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

void FilterThread::abortGmic()
{
  _gmicAbort = true;
}

void FilterThread::run()
{
  _startTime.start();
  _errorMessage.clear();
  _failed = false;
  if (!_images) {
    _images->assign(1);
    _imageNames->assign(1);
  }
  QString fullCommandLine;
  try {
    if (_messageMode == GmicQt::Quiet) {
      fullCommandLine = QString("v -");
    } else if (_messageMode >= GmicQt::VerboseLayerName && _messageMode <= GmicQt::VerboseLogFile) {
      fullCommandLine = QString("v -99");
    } else if (_messageMode == GmicQt::VeryVerboseConsole || _messageMode == GmicQt::VeryVerboseLogFile) {
      fullCommandLine = QString("v 0");
    } else if (_messageMode == GmicQt::DebugConsole || _messageMode == GmicQt::DebugLogFile) {
      fullCommandLine = QString("debug");
    }
    fullCommandLine += QString(" %1 %2").arg(_command).arg(_arguments);
    _gmicAbort = false;
    _gmicProgress = -1;
    if (_messageMode > GmicQt::Quiet) {
      std::fprintf(cimg::output(), "\n[gmic_qt] Command: %s\n", fullCommandLine.toLocal8Bit().constData());
      std::fflush(cimg::output());
    }

    gmic gmicInstance(_environment.isEmpty() ? 0 : QString("v - %1").arg(_environment).toLocal8Bit().constData(), GmicStdLib::Array.constData(), true);
    gmicInstance.set_variable("_host", GmicQt::HostApplicationShortname, '=');
    gmicInstance.run(fullCommandLine.toLocal8Bit().constData(), *_images, *_imageNames, &_gmicProgress, &_gmicAbort);
    _gmicStatus = gmicInstance.status;
  } catch (gmic_exception & e) {
    _images->assign();
    _imageNames->assign();
    const char * message = e.what();
    _errorMessage = message;
    if (_messageMode > GmicQt::Quiet) {
      std::fprintf(cimg::output(), "\n[gmic_qt]./error/ When running command '%s', this error occured:\n%s\n", fullCommandLine.toLocal8Bit().constData(), message);
      std::fflush(cimg::output());
    }
    _failed = true;
  }
}

void FilterThread::setCommand(const QString & command)
{
  _command = command;
}
