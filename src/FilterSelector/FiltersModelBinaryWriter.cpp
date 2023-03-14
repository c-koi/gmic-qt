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
#include "FilterSelector/FiltersModelBinaryWriter.h"
#include "Common.h"
#include "FilterSelector/FiltersModel.h"
#include "GmicQt.h"

#include <QDataStream>
#include <QFile>
#include <QMap>
#include <QString>

namespace GmicQt
{

FiltersModelBinaryWriter::FiltersModelBinaryWriter(const FiltersModel & model) : _model(model) {}

bool FiltersModelBinaryWriter::write(const QString & filename, const QByteArray & hash)
{
  TIMING;
  QFile file(filename);
  if (!file.open(QFile::WriteOnly)) {
    return false;
  }
  QDataStream stream(&file);

  stream << (quint32)0x03300330;
  stream << (quint32)100;
  stream.setVersion(QDataStream::Qt_5_0);

  stream << hash;

  QMap<QString, FiltersModel::Filter>::const_iterator it = _model._hash2filter.cbegin();
  const QMap<QString, FiltersModel::Filter>::const_iterator end = _model._hash2filter.cend();

  while (it != end) {
    stream << it.value()._name.toUtf8();
    stream << it.value()._plainText.toUtf8();
    stream << it.value()._translatedPlainText.toUtf8();
    writeStringList(it.value()._path, stream);
    writeStringList(it.value()._plainPath, stream);
    writeStringList(it.value()._translatedPlainPath, stream);
    stream << it.value()._command.toUtf8();
    stream << it.value()._previewCommand.toUtf8();
    stream << it.value()._defaultInputMode;
    stream << it.value()._parameters.toUtf8();
    stream << it.value()._previewFactor;
    stream << it.value()._isAccurateIfZoomed;
    stream << it.value()._previewFromFullImage;
    stream << it.value()._hash.toUtf8();
    stream << it.value()._isWarning;
    ++it;
  }

  TIMING;
  return true;
}

} // namespace GmicQt
