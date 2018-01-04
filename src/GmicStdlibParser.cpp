/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file GmicStdlibParser.cpp
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
#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QList>
#include <QString>
#include "gmic.h"
#include "Common.h"
#include "GmicStdlibParser.h"

// TODO : Change name of this class!

QByteArray GmicStdLibParser::GmicStdlib;

GmicStdLibParser::GmicStdLibParser()
{
}

void
GmicStdLibParser::loadStdLib()
{
  QFile stdlib(QString("%1update%2.gmic").arg(GmicQt::path_rc(false)).arg(gmic_version));
  if ( ! stdlib.open(QFile::ReadOnly) ) {
    gmic_image<char> stdlib_h = gmic::decompress_stdlib();
    GmicStdlib = QByteArray::fromRawData(stdlib_h,stdlib_h.size());
    GmicStdlib[GmicStdlib.size()-1] = '\n';
  } else {
    GmicStdlib = stdlib.readAll();
  }
}

QStringList GmicStdLibParser::parseStatus(QString str)
{
  if ( !str.startsWith(QChar(24)) || !str.endsWith(QChar(25 )) ) {
    return QStringList();
  }
  QList<QString> list = str.split(QString("%1%2").arg(QChar(25)).arg(QChar(24)));
  list[0].replace(QChar(24),QString());
  list.back().replace(QChar(25),QString());
  return list;
}
