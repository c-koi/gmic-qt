/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterSyncRunner.cpp
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
#include "FilterSyncRunner.h"
#include <QDebug>
#include <QThread>
#include <iostream>
#include "FilterThread.h"
#include "GmicStdlib.h"
#include "Logger.h"
#include "Misc.h"
#include "PersistentMemory.h"
#include "Settings.h"
#include "gmic.h"

namespace GmicQt
{

FilterSyncRunner::FilterSyncRunner(QObject * parent, const QString & command, const QString & arguments, const QString & environment, OutputMessageMode mode)
    : QObject(parent), _command(command), _arguments(arguments), _environment(environment), //
      _images(new cimg_library::CImgList<float>),                                           //
      _imageNames(new cimg_library::CImgList<char>),                                        //
      _persistentMemoryOuptut(new cimg_library::CImg<char>)
{
#ifdef _IS_MACOS_
  static bool stackSize8MB = false;
  if (!stackSize8MB) {
    QThread::currentThread()->setStackSize(8 * 1024 * 1024);
    stackSize8MB = true;
  }
#endif
  _gmicAbort = false;
  _failed = false;
  _gmicProgress = 0.0f;
}

FilterSyncRunner::~FilterSyncRunner()
{
  delete _images;
  delete _imageNames;
  delete _persistentMemoryOuptut;
}

void FilterSyncRunner::setArguments(const QString & str)
{
  _arguments = str;
}

void FilterSyncRunner::setImageNames(const cimg_library::CImgList<char> & imageNames)
{
  *_imageNames = imageNames;
}

void FilterSyncRunner::swapImages(cimg_library::CImgList<float> & images)
{
  _images->swap(images);
}

void FilterSyncRunner::setInputImages(const cimg_library::CImgList<float> & list)
{
  *_images = list;
}

const cimg_library::CImgList<float> & FilterSyncRunner::images() const
{
  return *_images;
}

const cimg_library::CImgList<char> & FilterSyncRunner::imageNames() const
{
  return *_imageNames;
}

cimg_library::CImg<char> & FilterSyncRunner::persistentMemoryOutput()
{
  return *_persistentMemoryOuptut;
}

QStringList FilterSyncRunner::gmicStatus() const
{
  return FilterThread::status2StringList(_gmicStatus);
}

QList<int> FilterSyncRunner::parametersVisibilityStates() const
{
  return FilterThread::status2Visibilities(_gmicStatus);
}

QString FilterSyncRunner::errorMessage() const
{
  return _errorMessage;
}

bool FilterSyncRunner::failed() const
{
  return _failed;
}

bool FilterSyncRunner::aborted() const
{
  return _gmicAbort;
}

float FilterSyncRunner::progress() const
{
  return _gmicProgress;
}

QString FilterSyncRunner::fullCommand() const
{
  QString result = _command;
  appendWithSpace(result, _arguments);
  return result;
}

void FilterSyncRunner::setLogSuffix(const QString & text)
{
  _logSuffix = text;
}

void FilterSyncRunner::abortGmic()
{
  _gmicAbort = true;
}

void FilterSyncRunner::run()
{
  _errorMessage.clear();
  _failed = false;
  QString fullCommandLine;
  try {
    fullCommandLine = commandFromOutputMessageMode(Settings::outputMessageMode());
    appendWithSpace(fullCommandLine, _command);
    appendWithSpace(fullCommandLine, _arguments);
    _gmicAbort = false;
    _gmicProgress = -1;
    Logger::log(fullCommandLine, _logSuffix, true);
    gmic gmicInstance(_environment.isEmpty() ? nullptr : QString("%1").arg(_environment).toLocal8Bit().constData(), GmicStdLib::Array.constData(), true, 0, 0, 0.f);
    gmicInstance.set_variable("_persistent", PersistentMemory::image());
    gmicInstance.set_variable("_host", '=', GmicQtHost::ApplicationShortname);
    gmicInstance.set_variable("_tk", '=', "qt");
    gmicInstance.run(fullCommandLine.toLocal8Bit().constData(), *_images, *_imageNames, &_gmicProgress, &_gmicAbort);
    _gmicStatus = QString::fromLocal8Bit(gmicInstance.status);
    gmicInstance.get_variable("_persistent").move_to(*_persistentMemoryOuptut);
  } catch (gmic_exception & e) {
    _images->assign();
    _imageNames->assign();
    const char * message = e.what();
    _errorMessage = message;
    Logger::error(QString("When running command '%1', this error occurred:\n%2").arg(fullCommandLine).arg(message), true);
    _failed = true;
  }
}

} // namespace GmicQt
