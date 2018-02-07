/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FavesModelWriter.cpp
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
#include "FavesModelWriter.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QTextStream>
#include <iostream>
#include "Utils.h"

FavesModelWriter::FavesModelWriter(const FavesModel & model) : _model(model)
{
}

FavesModelWriter::~FavesModelWriter()
{
}

void FavesModelWriter::writeFaves()
{
  QString jsonFilename(QString("%1%2").arg(GmicQt::path_rc(true)).arg("gmic_qt_faves.json"));
  // Create JSON array
  QJsonArray array;
  FavesModel::const_iterator itFave = _model.cbegin();
  while (itFave != _model.cend()) {
    QJsonObject object = faveToJsonObject(*itFave);
    array.append(object);
    ++itFave;
  }
  if (array.isEmpty()) { // Backup
    QFile::copy(jsonFilename, jsonFilename + ".bak");
  }
  // Save JSON array
  QFile jsonFile(jsonFilename);
  if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    QJsonDocument jsonDoc(array);
    if (jsonFile.write(jsonDoc.toJson()) != -1) {
      // Cleanup 2.0.0 pre-release files
      QString obsoleteFilename(QString("%1%2").arg(GmicQt::path_rc(false)).arg("gmic_qt_faves"));
      QFile::remove(obsoleteFilename);
      QFile::remove(obsoleteFilename + ".bak");
    }
  } else {
    std::cerr << "[gmic_qt] Error: cannot open/create file " << jsonFilename.toStdString() << std::endl;
  }
}

QJsonObject FavesModelWriter::faveToJsonObject(const FavesModel::Fave & fave)
{
  QJsonObject object;
  object["Name"] = fave.name();
  object["originalName"] = fave.originalName();
  object["command"] = fave.command();
  object["preview"] = fave.previewCommand();
  QJsonArray array;
  for (const QString & str : fave.defaultValues()) {
    array.push_back(str);
  }
  object["defaultParameters"] = array;
  return object;
}
