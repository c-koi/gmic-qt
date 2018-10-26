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
#include <QDebug>
#include <QFileInfo>
#include <QRegExp>
#include <QString>
#include "Common.h"
#include "Host/host.h"
#include "gmic.h"

#ifdef _IS_WINDOWS_
#include <windows.h>
#include <tlhelp32.h>
#endif
#ifdef _IS_LINUX_
#include <unistd.h>
#endif

namespace GmicQt
{
const QString & path_rc(bool create)
{
  QString qpath = QString::fromLocal8Bit(gmic::path_rc());
  QFileInfo dir(qpath);
  static QString result;
  if (dir.isDir()) {
    result = qpath;
    return result;
  }
  if (!create || !gmic::init_rc()) {
    result.clear();
  } else {
    result = QString::fromLocal8Bit(gmic::path_rc());
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
                 .arg(GmicQt::HostApplicationName.isEmpty() ? QString() : QString("for %1 ").arg(GmicQt::HostApplicationName))
                 .arg(cimg_library::cimg::stros())
                 .arg(sizeof(void *) == 8 ? 64 : 32)
                 .arg(GmicQt::gmicVersionString());
  }
  return result;
}

const QString & pluginCodeName()
{
  static QString result;
  if (result.isEmpty()) {
    result = GmicQt::HostApplicationName.isEmpty() ? QString("gmic_qt") : QString("gmic_%1_qt").arg(QString(GmicQt::HostApplicationShortname).toLower());
  }
  return result;
}

const char * commandFromOutputMessageMode(OutputMessageMode mode)
{
  static const char commands[][6] = {"v -", "v -99", "v 0", "debug"};
  if (mode == GmicQt::Quiet) {
    return commands[0];
  } else if (mode >= GmicQt::VerboseLayerName && mode <= GmicQt::VerboseLogFile) {
    return commands[1];
  } else if (mode == GmicQt::VeryVerboseConsole || mode == GmicQt::VeryVerboseLogFile) {
    return commands[2];
  } else if (mode == GmicQt::DebugConsole || mode == GmicQt::DebugLogFile) {
    return commands[3];
  }
  return commands[0];
}

void downcaseCommandTitle(QString & title)
{
  QMap<int, QString> acronyms;
  // Acronyms
  QRegExp re("([A-Z0-9]{2,255})");
  int index = 0;
  while ((index = re.indexIn(title, index)) != -1) {
    QString pattern = re.cap(0);
    acronyms[index] = pattern;
    index += pattern.length();
  }

  // 3D
  re.setPattern("([1-9])[dD] ");
  if ((index = re.indexIn(title, 0)) != -1) {
    acronyms[index] = re.cap(1) + "d ";
  }

  // B&amp;W
  re.setPattern("(B&amp;W|[ \\[]Lab|[ \\[]YCbCr)");
  index = 0;
  while ((index = re.indexIn(title, index)) != -1) {
    acronyms[index] = re.cap(1);
    index += re.cap(1).length();
  }

  // Uppercase letter in last position, after a space
  re.setPattern(" ([A-Z])$");
  if ((index = re.indexIn(title, 0)) != -1) {
    acronyms[index] = re.cap(0);
  }
  title = title.toLower();
  QMap<int, QString>::const_iterator it = acronyms.cbegin();
  while (it != acronyms.cend()) {
    title.replace(it.key(), it.value().length(), it.value());
    ++it;
  }
  title.front() = title.front().toUpper();
}

} // namespace GmicQt
