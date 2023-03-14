/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersModelBinaryWriter.h
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
#ifndef GMIC_QT_FILTERSMODELBINARYWRITER_H
#define GMIC_QT_FILTERSMODELBINARYWRITER_H

class QByteArray;
#include <QDataStream>
#include <QList>
#include <QString>

namespace GmicQt
{

class FiltersModel;

class FiltersModelBinaryWriter {
public:
  FiltersModelBinaryWriter(const FiltersModel & model);
  bool write(const QString & filename, const QByteArray & hash);

private:
  static inline void writeStringList(const QList<QString> & list, QDataStream & stream);
  const FiltersModel & _model;
};

void FiltersModelBinaryWriter::writeStringList(const QList<QString> & list, QDataStream & stream)
{
  stream << (quint8)list.size();
  for (const QString & str : list) {
    stream << str.toUtf8();
  }
}

} // namespace GmicQt

#endif // GMIC_QT_FILTERSMODELBINARYWRITER_H
