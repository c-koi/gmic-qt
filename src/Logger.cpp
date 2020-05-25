/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file Logger.cpp
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
#include "Logger.h"
#include <QDebug>
#include <QString>
#include <QStringList>
#include "Common.h"
#include "Utils.h"
#include "gmic_qt.h"
#include "gmic.h"

FILE * Logger::_logFile = nullptr;
Logger::Mode Logger::_currentMode = Logger::StandardOutput;

void Logger::setMode(const GmicQt::OutputMessageMode mode)
{
  if ((mode == GmicQt::VerboseLogFile) || (mode == GmicQt::VeryVerboseLogFile) || (mode == GmicQt::DebugLogFile)) {
    setMode(Logger::File);
  } else {
    setMode(Logger::StandardOutput);
  }
}

void Logger::setMode(const Logger::Mode mode)
{
  if (mode == _currentMode) {
    return;
  }
  if (mode == StandardOutput) {
    if (_logFile) {
      fclose(_logFile);
    }
    _logFile = nullptr;
    cimg_library::cimg::output(stdout);
  } else {
    QString filename = QString("%1gmic_qt_log").arg(GmicQt::path_rc(true));
    _logFile = fopen(filename.toLocal8Bit().constData(), "a");
    cimg_library::cimg::output(_logFile ? _logFile : stdout);
  }
  _currentMode = mode;
}

void Logger::clear()
{
  Mode mode = _currentMode;
  if (mode == File) {
    setMode(StandardOutput);
  }
  QString filename = QString("%1gmic_qt_log").arg(GmicQt::path_rc(true));
  FILE * dummyFile = fopen(filename.toLocal8Bit().constData(), "w");
  fclose(dummyFile);
  setMode(mode);
}

void Logger::log(const QString & message, bool space)
{
  log(message, QString(), space);
}

void Logger::log(const QString & message, const QString & hint, bool space)
{
  QString text = message;
  while (!text.isEmpty() && text[text.size() - 1].isSpace()) {
    text.chop(1);
  }
  QStringList lines = text.split("\n", QString::KeepEmptyParts);

  QString prefix = QString("[%1]").arg(GmicQt::pluginCodeName());
  prefix += hint.isEmpty() ? " " : QString("./%1/ ").arg(hint);

  if (space) {
    std::fprintf(cimg_library::cimg::output(), "\n");
  }
  for (const QString & line : lines) {
    std::fprintf(cimg_library::cimg::output(), "%s\n", (prefix + line).toLocal8Bit().constData());
  }
  std::fflush(cimg_library::cimg::output());
}

void Logger::error(const QString & message, bool space)
{
  log(message, "error", space);
}

void Logger::warning(const QString & message, bool space)
{
  log(message, "warning", space);
}

void Logger::note(const QString & message, bool space)
{
  log(message, "note", space);
}
