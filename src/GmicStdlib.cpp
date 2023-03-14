/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file GmicStdlib.cpp
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
#include "GmicStdlib.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include "GmicQt.h"
#include "Utils.h"
#include "gmic.h"

namespace GmicQt
{

QByteArray GmicStdLib::Array;

void GmicStdLib::loadStdLib() // TODO : Remove
{
  QString path = QString("%1update%2.gmic").arg(gmicConfigPath(false)).arg(gmic_version);
  QFileInfo info(path);
  QFile stdlib(path);
  if ((info.size() == 0) || !stdlib.open(QFile::ReadOnly)) {
    gmic_image<char> stdlib_h = gmic::decompress_stdlib();
    Array = QByteArray::fromRawData(stdlib_h, stdlib_h.size());
    Array[Array.size() - 1] = '\n';
  } else {
    Array = stdlib.readAll();
  }
}

QString GmicStdLib::substituteSourceVariables(QString text)
{
  text.replace("$HOME", QDir::home().absolutePath());
  text.replace("$VERSION", QString::number(GmicQt::GmicVersion));
#ifdef _IS_WINDOWS_
  text.replace("%APPDATA%", QString::fromLocal8Bit(qgetenv("APPDATA")));
#endif
  return text;
}

QStringList GmicStdLib::substituteSourceVariables(QStringList list)
{
  QStringList result;
  for (const QString & str : list) {
    result << substituteSourceVariables(str);
  }
  return result;
}

QByteArray GmicStdLib::hash()
{
  return QCryptographicHash::hash(Array, QCryptographicHash::Sha1);
}

} // namespace GmicQt
