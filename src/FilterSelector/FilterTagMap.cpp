/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterTagMap.h
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
#include "FilterTagMap.h"
#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include "Common.h"
#include "Globals.h"
#include "GmicQt.h"
#include "Logger.h"
#include "Utils.h"

namespace GmicQt
{

QSet<QString> FiltersTagMap::_colorToHashes[static_cast<unsigned int>(TagColor::Count)];

TagColor FiltersTagMap::filterTag(const QString & hash)
{
  return TagColor::None;
}

void FiltersTagMap::setColor(const QString & hash, TagColor color)
{
  for (QSet<QString> & hashSet : _colorToHashes) {
    if (hashSet.remove(hash) && color == TagColor::None) {
      return;
    }
  }
  if (color != TagColor::None) {
    _colorToHashes[static_cast<unsigned int>(color)].insert(hash);
  }
}

void FiltersTagMap::load()
{
  for (QSet<QString> & hashes : _colorToHashes) {
    hashes.clear();
  }
  QString jsonFilename = QString("%1%2").arg(gmicConfigPath(true), FILTERS_TAGS_FILENAME);
  QFile jsonFile(jsonFilename);
  if (!jsonFile.exists()) {
    return;
  }
  if (jsonFile.open(QFile::ReadOnly)) {
#ifdef _GMIC_QT_DEBUG_
    QJsonDocument jsonDoc;
    QByteArray allFile = jsonFile.readAll();
    if (allFile.startsWith("{")) {
      jsonDoc = QJsonDocument::fromJson(allFile);
    } else {
      jsonDoc = QJsonDocument::fromJson(qUncompress(allFile));
    }
#else
    QJsonDocument jsonDoc = QJsonDocument::fromJson(qUncompress(jsonFile.readAll()));
#endif
    if (jsonDoc.isNull()) {
      Logger::warning(QString("Cannot parse ") + jsonFilename);
      Logger::warning("Fiter tags are lost!");
    } else {
      if (!jsonDoc.isObject()) {
        Logger::error(QString("JSON file format is not correct (") + jsonFilename + ")");
      } else {
        QJsonObject documentObject = jsonDoc.object();

        for (int color = (int)TagColor::None + 1; color != (int)TagColor::Count; ++color) {
          QJsonObject::const_iterator it = documentObject.find(TagColorNames[color]);
          QSet<QString> & hashes = _colorToHashes[color];
          if (it != documentObject.constEnd()) {
            QJsonArray array = it.value().toArray();
            for (const QJsonValueRef & value : array) {
              hashes.insert(value.toString());
            }
          }
        }
      }
    }
  } else {
    Logger::error("Cannot open " + jsonFilename);
    Logger::error("Parameters cannot be restored");
  }
}

void FiltersTagMap::save()
{
  QJsonObject documentObject;

  for (int color = (int)TagColor::None + 1; color != (int)TagColor::Count; ++color) {
    QJsonArray array;
    const QSet<QString> & hashes = _colorToHashes[color];
    for (const QString & hash : hashes) {
      array.push_back(QJsonValue(hash));
    }
    documentObject.insert(TagColorNames[color], array);
  }

  QJsonDocument jsonDoc(documentObject);
  QString jsonFilename = QString("%1%2").arg(gmicConfigPath(true), FILTERS_TAGS_FILENAME);
  QFile jsonFile(jsonFilename);
  if (QFile::exists(jsonFilename)) {
    QString bakFilename = QString("%1%2").arg(gmicConfigPath(false), FILTERS_TAGS_FILENAME ".bak");
    QFile::remove(bakFilename);
    QFile::copy(jsonFilename, bakFilename);
  }

  qint64 count = -1;
  if (jsonFile.open(QFile::WriteOnly | QFile::Truncate)) {
#ifdef _GMIC_QT_DEBUG_
    count = jsonFile.write(jsonDoc.toJson());
#else
    count = jsonFile.write(qCompress(jsonDoc.toJson(QJsonDocument::Compact)));
#endif
    jsonFile.close();
  }

  if (count == -1) {
    Logger::error("Cannot write " + jsonFilename);
    Logger::error("Parameters cannot be saved");
  }
}

} // namespace GmicQt
