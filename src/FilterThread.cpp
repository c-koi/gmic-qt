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
#include "FilterParameters/AbstractParameter.h"
#include "GmicStdlib.h"
#include "Logger.h"
#include "Misc.h"
#include "PersistentMemory.h"
#include "Settings.h"
#include "gmic.h"

namespace GmicQt
{

FilterThread::FilterThread(QObject * parent, const QString & command, const QString & arguments, const QString & environment)
    : QThread(parent), _command(command), _arguments(arguments), _environment(environment), //
      _images(new cimg_library::CImgList<float>),                                           //
      _imageNames(new cimg_library::CImgList<char>),                                        //
      _persistentMemoryOuptut(new cimg_library::CImg<char>)
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
  delete _persistentMemoryOuptut;
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

cimg_library::CImg<char> & FilterThread::persistentMemoryOutput()
{
  return *_persistentMemoryOuptut;
}

QStringList FilterThread::status2StringList(const QString & status)
{
  // Check if status matches something like "{...}{...}_1{...}_0"
  QRegExp statusRegExp(QString("^") + QChar(gmic_lbrace) + "(.*)" + QChar(gmic_rbrace) + QString("(_[012][+*-]?)?$"));
  QRegExp statusSeparatorRegExp(QChar(gmic_rbrace) + QString("(_[012][+*-]?)?") + QChar(gmic_lbrace));
  if (status.isEmpty()) {
    return QStringList();
  }
  if (statusRegExp.indexIn(status) == -1) {
    // TRACE << "Warning: Incorrect status syntax " << status;
    return QStringList();
  }
  QStringList list = statusRegExp.cap(1).split(statusSeparatorRegExp);
  if (!list.isEmpty()) {
    QStringList::iterator it = list.begin();
    while (it != list.end()) {
      QByteArray array = it->toLocal8Bit();
      gmic::strreplace_fw(array.data());
      *it++ = QString::fromLocal8Bit(array);
    }
  }
  return list;
}

QList<int> FilterThread::status2Visibilities(const QString & status)
{
  if (status.isEmpty()) {
    return QList<int>();
  }
  // Check if status matches something like "{...}{...}_1{...}_0"
  QRegExp statusRegExp(QString("^") + QChar(gmic_lbrace) + "(.*)" + QChar(gmic_rbrace) + QString("(_[012])?$"));
  if (!status.isEmpty() && statusRegExp.indexIn(status) == -1) {
    // TRACE << "Incorrect status syntax " << status;
    return QList<int>();
  }

  QByteArray ba = status.toLocal8Bit();
  const char * pc = ba.constData();
  const char * limit = pc + ba.size();

  QList<int> result;
  while (pc < limit) {
    if (*pc == gmic_rbrace) {
      if ((pc < limit - 2) && pc[1] == '_' && pc[2] >= '0' && pc[2] <= '2' && (!pc[3] || pc[3] == gmic_lbrace)) {
        result.push_back(pc[2] - '0'); // AbstractParameter::VisibilityState
        pc += 3;
      } else if (!pc[1] || (pc[1] == gmic_lbrace)) {
        result.push_back((int)AbstractParameter::VisibilityState::Unspecified);
        ++pc;
      } else {
        // TRACE << "Ignoring status" << qPrintable(status);
        return QList<int>();
      }
    } else {
      ++pc;
    }
  }
  return result;
}

QStringList FilterThread::gmicStatus() const
{
  return status2StringList(_gmicStatus);
}

QList<int> FilterThread::parametersVisibilityStates() const
{
  return status2Visibilities(_gmicStatus);
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
  return static_cast<int>(_startTime.elapsed());
}

float FilterThread::progress() const
{
  return _gmicProgress;
}

QString FilterThread::fullCommand() const
{
  QString result = _command;
  appendWithSpace(result, _arguments);
  return result;
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
    fullCommandLine = commandFromOutputMessageMode(Settings::outputMessageMode());
    appendWithSpace(fullCommandLine, _command);
    appendWithSpace(fullCommandLine, _arguments);
    _gmicAbort = false;
    _gmicProgress = -1;
    Logger::log(fullCommandLine, _logSuffix, true);
    gmic gmicInstance(_environment.isEmpty() ? nullptr : QString("%1").arg(_environment).toLocal8Bit().constData(), GmicStdLib::Array.constData(), true, nullptr, nullptr, 0.0f);
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
