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
#include "GmicQt.h"
#include "Settings.h"
#include "Utils.h"
#include "gmic.h"

namespace GmicQt
{

FILE * Logger::_logFile = nullptr;
Logger::Mode Logger::_currentMode = Logger::Mode::StandardOutput;

void Logger::setMode(const OutputMessageMode mode)
{
  if ((mode == OutputMessageMode::VerboseLogFile) || (mode == OutputMessageMode::VeryVerboseLogFile) || (mode == OutputMessageMode::DebugLogFile)) {
    setMode(Logger::Mode::File);
  } else {
    setMode(Logger::Mode::StandardOutput);
  }
}

void Logger::setMode(const Logger::Mode mode)
{
  if (mode == _currentMode) {
    return;
  }
  if (mode == Mode::StandardOutput) {
    if (_logFile) {
      fclose(_logFile);
    }
    _logFile = nullptr;
    gmic_library::cimg::output(stdout);
  } else {
    QString filename = QString("%1gmic_qt_log").arg(gmicConfigPath(true));
    _logFile = fopen(filename.toLocal8Bit().constData(), "a");
    gmic_library::cimg::output(_logFile ? _logFile : stdout);
  }
  _currentMode = mode;
}

void Logger::clear()
{
  Mode mode = _currentMode;
  if (mode == Mode::File) {
    setMode(Mode::StandardOutput);
  }
  QString filename = QString("%1gmic_qt_log").arg(gmicConfigPath(true));
  FILE * dummyFile = fopen(filename.toLocal8Bit().constData(), "w");
  if (dummyFile) {
    fclose(dummyFile);
  }
  setMode(mode);
}

void Logger::log(const QString & message, bool space)
{
  log(message, QString(), space);
}

void Logger::log(const QString & message, const QString & hint, bool space)
{
  if (Settings::outputMessageMode() == OutputMessageMode::Quiet) {
    return;
  }
  QString text = message;
  while (!text.isEmpty() && text[text.size() - 1].isSpace()) {
    text.chop(1);
  }
  QStringList lines = text.split("\n", QT_KEEP_EMPTY_PARTS);

  QString prefix = QString("[%1]").arg(pluginCodeName());
  prefix += hint.isEmpty() ? " " : QString("./%1/ ").arg(hint);

  if (space) {
    std::fprintf(gmic_library::cimg::output(), "\n");
  }
  for (const QString & line : lines) {
    std::fprintf(gmic_library::cimg::output(), "%s\n", (prefix + line).toLocal8Bit().constData());
  }
  std::fflush(gmic_library::cimg::output());
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

} // namespace GmicQt
