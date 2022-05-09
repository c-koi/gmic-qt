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
#include <QList>
#include "Common.h"
#include "Globals.h"
#include "GmicQt.h"
#include "Logger.h"
#include "Utils.h"

namespace GmicQt
{

QMap<QString, TagColorSet> FiltersTagMap::_hashesToColors;

TagColorSet FiltersTagMap::filterTags(const QString & hash)
{
  auto it = _hashesToColors.find(hash);
  if (it == _hashesToColors.end()) {
    return TagColorSet::Empty;
  }
  return it.value();
}

void FiltersTagMap::setFilterTags(const QString & hash, const TagColorSet & colors)
{
  if (colors.isEmpty()) {
    _hashesToColors.remove(hash);
    return;
  }
  _hashesToColors[hash] = colors;
}

void FiltersTagMap::load()
{
  _hashesToColors.clear();
  QString jsonFilename = QString("%1%2").arg(gmicConfigPath(false), FILTERS_TAGS_FILENAME);
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
      Logger::warning("Filter tags are lost!");
    } else {
      if (!jsonDoc.isObject()) {
        Logger::error(QString("JSON file format is not correct (") + jsonFilename + ")");
      } else {
        QJsonObject documentObject = jsonDoc.object();
        for (QJsonObject::const_iterator it = documentObject.constBegin(); //
             it != documentObject.constEnd();                              //
             ++it) {
          _hashesToColors[it.key()] = TagColorSet(it.value().toInt());
        }
      }
    }
  } else {
    Logger::error("Cannot open " + jsonFilename);
    Logger::error("Tags cannot be restored");
  }
}

void FiltersTagMap::save()
{
  QJsonObject documentObject;
  auto it = _hashesToColors.begin();
  while (it != _hashesToColors.end()) {
    documentObject.insert(it.key(), QJsonValue(int(it.value().mask())));
    ++it;
  }
  QJsonDocument jsonDoc(documentObject);
  QString jsonFilename = QString("%1%2").arg(gmicConfigPath(true), FILTERS_TAGS_FILENAME);
  if (QFile::exists(jsonFilename)) {
    QString bakFilename = QString("%1%2").arg(gmicConfigPath(false), FILTERS_TAGS_FILENAME ".bak");
    QFile::remove(bakFilename);
    QFile::copy(jsonFilename, bakFilename);
  }

#ifdef _GMIC_QT_DEBUG_
  const bool ok = safelyWrite(jsonDoc.toJson(), jsonFilename);
#else
  const bool ok = safelyWrite(qCompress(jsonDoc.toJson(QJsonDocument::Compact)), jsonFilename);
#endif
  if (!ok) {
    Logger::error("Cannot write " + jsonFilename);
    Logger::error("Parameters cannot be saved");
  }
}

TagColorSet FiltersTagMap::usedColors(int * count)
{
  TagColorSet all;
  auto it = _hashesToColors.cbegin();
  if (count) {
    memset(count, 0, sizeof(int) * int(TagColor::Count));
    while (it != _hashesToColors.cend()) {
      TagColorSet colors = it.value();
      for (TagColor color : colors) {
        ++count[int(color)];
      }
      all |= colors;
      ++it;
    }
  } else {
    while (it != _hashesToColors.cend()) {
      all |= it.value();
      ++it;
    }
  }
  return all;
}

void FiltersTagMap::removeAllTags(TagColor color)
{
  QList<QString> toBeRemoved;
  auto it = _hashesToColors.begin();
  while (it != _hashesToColors.end()) {
    it.value() -= color;
    if (it.value().isEmpty()) {
      toBeRemoved.push_back(it.key());
    }
    ++it;
  }
  for (const QString & hash : toBeRemoved) {
    _hashesToColors.remove(hash);
  }
}

void FiltersTagMap::clearFilterTag(const QString & hash, TagColor color)
{
  auto it = _hashesToColors.find(hash);
  if (it == _hashesToColors.end()) {
    return;
  }
  it.value() -= color;
  if (it.value().isEmpty()) {
    _hashesToColors.erase(it);
  }
}

void FiltersTagMap::setFilterTag(const QString & hash, TagColor color)
{
  _hashesToColors[hash] += color;
}

void FiltersTagMap::toggleFilterTag(const QString & hash, TagColor color)
{
  _hashesToColors[hash].toggle(color);
}

void FiltersTagMap::remove(const QString & hash)
{
  _hashesToColors.remove(hash);
}
} // namespace GmicQt
