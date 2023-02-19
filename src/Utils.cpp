/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file Utils.cpp
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

#include "Utils.h"
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryFile>
#include "Common.h"
#include "Host/GmicQtHost.h"
#include "Logger.h"
#include "Gmic.h"

#ifdef _IS_WINDOWS_
#include <windows.h>
#include <tlhelp32.h>
#endif
#ifdef _IS_LINUX_
#include <unistd.h>
#endif

namespace GmicQt
{

const QString & gmicConfigPath(bool create)
{
#if defined(Q_OS_ANDROID)
  QString baseAppPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  const QString qpath = QString::fromUtf8(gmic::path_rc(qPrintable(baseAppPath)));
#elif defined(_IS_WINDOWS_)
  const QString qpath = QString::fromUtf8(gmic::path_rc());
#else
  const QString qpath = QString::fromLocal8Bit(gmic::path_rc());
#endif
  static QString result;
  QFileInfo pathInfo(qpath);
  if (pathInfo.isDir() || (create && gmic::init_rc())) {
    result = qpath;
  } else {
    result.clear();
  }
  return result;
}

unsigned int host_app_pid()
{
#if defined(_IS_LINUX_)
  return static_cast<int>(getppid());
#elif defined(_IS_WINDOWS_)
  HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32 pe;
  memset(&pe, 0, sizeof(PROCESSENTRY32));
  pe.dwSize = sizeof(PROCESSENTRY32);
  DWORD pid = GetCurrentProcessId();
  if (Process32First(h, &pe)) {
    do {
      if (pe.th32ProcessID == pid) {
        CloseHandle(h);
        return static_cast<unsigned int>(pe.th32ParentProcessID);
      }
    } while (Process32Next(h, &pe));
  }
  CloseHandle(h);
  return static_cast<unsigned int>(pid); // Process own id if no parent was found
#else
  return 0;
#endif
}

const QString & pluginFullName()
{
#ifdef gmic_prerelease
#define BETA_SUFFIX "_pre#" gmic_prerelease
#else
#define BETA_SUFFIX ""
#endif
  static QString result;
  if (result.isEmpty()) {
    result = QString("G'MIC-Qt %1- %2 %3 bits - %4" BETA_SUFFIX)
                 .arg(GmicQtHost::ApplicationName.isEmpty() ? QString() : QString("for %1 ").arg(GmicQtHost::ApplicationName))
                 .arg(gmic_library::cimg::stros())
                 .arg(sizeof(void *) == 8 ? 64 : 32)
                 .arg(gmicVersionString());
  }
  return result;
}

const QString & pluginCodeName()
{
  static QString result;
  if (result.isEmpty()) {
    result = GmicQtHost::ApplicationName.isEmpty() ? QString("gmic_qt") : QString("gmic_%1_qt").arg(QString(GmicQtHost::ApplicationShortname).toLower());
  }
  return result;
}

bool touchFile(const QString & path)
{
  QFile file(path);
  if (!file.open(QFile::ReadWrite)) {
    return false;
  }
  const auto size = file.size();
  file.resize(size + 1);
  file.resize(size);
  return true;
}

bool writeAll(const QByteArray & array, QFile & file)
{
  qint64 toBeWritten = array.size();
  qint64 totalWritten = 0;
  qint64 writtenByCall = 0;
  qint64 maxBytesPerWrite = 10l << 20;
  const char * data = array.constData();
  do {
    writtenByCall = file.write(data, std::min(toBeWritten, maxBytesPerWrite));
    if (writtenByCall == -1) {
      Logger::error(QString("Could not properly write file %1 (%2/%3 bytes written)") //
                        .arg(file.fileName())
                        .arg(totalWritten)
                        .arg(array.size()));
      return false;
    }
    data += writtenByCall;
    totalWritten += writtenByCall;
    toBeWritten -= writtenByCall;
  } while (toBeWritten);
  file.flush();
  return true;
}

bool safelyWrite(const QByteArray & array, const QString & filename)
{
  QString directory = QFileInfo(filename).absoluteDir().absolutePath();
  if (!QFileInfo(directory).isWritable()) {
    Logger::error(QString("Folder is not writable (%1)").arg(directory));
    return false;
  }
  QTemporaryFile temporary;
  temporary.setAutoRemove(false);
  const bool ok = temporary.open()                                              //
                  && writeAll(array, temporary)                                 //
                  && (!QFileInfo(filename).exists() || QFile::remove(filename)) //
                  && temporary.copy(filename);
  temporary.remove();
  return ok;
}
} // namespace GmicQt
