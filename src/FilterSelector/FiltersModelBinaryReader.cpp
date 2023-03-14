/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersModelBinaryReader.cpp
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
#include "FilterSelector/FiltersModelBinaryReader.h"
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include "Common.h"
#include "FilterSelector/FiltersModel.h"
#include "Logger.h"

namespace GmicQt
{

FiltersModelBinaryReader::FiltersModelBinaryReader(FiltersModel & model) : _model(model) {}

bool FiltersModelBinaryReader::read(const QString & filename)
{
  TIMING;
  QFile file(filename);
  if (!file.open(QFile::ReadOnly)) {
    return false;
  }
  QDataStream stream(&file);
  QByteArray hash;

  if (!readHeader(stream, hash)) {
    return false;
  }

#define READ_STRING(STR)                                                                                                                                                                               \
  stream >> array;                                                                                                                                                                                     \
  STR = QString::fromUtf8(array)

  FiltersModel::Filter filter;
  QByteArray array;
  quint8 inputMode;
  while (!stream.atEnd()) {
    READ_STRING(filter._name);
    READ_STRING(filter._plainText);
    READ_STRING(filter._translatedPlainText);
    readStringList(stream, filter._path);
    readStringList(stream, filter._plainPath);
    readStringList(stream, filter._translatedPlainPath);
    READ_STRING(filter._command);
    READ_STRING(filter._previewCommand);
    stream >> inputMode;
    filter._defaultInputMode = InputMode(inputMode);
    READ_STRING(filter._parameters);
    stream >> filter._previewFactor;
    stream >> filter._isAccurateIfZoomed;
    stream >> filter._previewFromFullImage;
    READ_STRING(filter._hash);
    stream >> filter._isWarning;
    _model._hash2filter[filter._hash] = filter;
  }
  TIMING;
  return true;
}

QByteArray FiltersModelBinaryReader::readHash(const QString & filename)
{
  QByteArray hash;
  QFile file(filename);
  if (file.open(QFile::ReadOnly)) {
    QDataStream stream(&file);
    readHeader(stream, hash);
  }
  return hash;
}

bool FiltersModelBinaryReader::readHeader(QDataStream & stream, QByteArray & hash)
{
  quint32 magic;
  stream >> magic;
  if (magic != (quint32)0x03300330) {
    Logger::warning("Filters binary cache: wrong magic number");
    return false;
  }
  quint32 version;
  stream >> version;
  if (version <= (quint32)100) {
    stream.setVersion(QDataStream::Qt_5_0);
  } else {
    Logger::warning("Filters binary cache: unsupported version");
    return false;
  }

  stream >> hash;
  if (hash.isEmpty()) {
    Logger::warning("Filters binary cache: cannot read hash");
    return false;
  }
  return true;
}

} // namespace GmicQt
