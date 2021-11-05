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
#include "Common.h"
#include "Globals.h"
#include "GmicQt.h"
#include "Logger.h"
#include "Utils.h"

namespace GmicQt
{

QMap<QString, unsigned int> FiltersTagMap::_hashesToColors;

QVector<TagColor> FiltersTagMap::filterTags(const QString & hash)
{
  QMap<QString, unsigned int>::const_iterator it = _hashesToColors.find(hash);
  if (it == _hashesToColors.cend()) {
    return QVector<TagColor>();
  }
  return uint2colors(it.value());
}

void FiltersTagMap::setFilterTags(const QString & hash, const QVector<TagColor> & colors)
{
  if (colors.isEmpty()) {
    _hashesToColors.remove(hash);
    return;
  }
  _hashesToColors[hash] = colors2uint(colors);
}

void FiltersTagMap::load()
{
  _hashesToColors.clear();
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
        for (QJsonObject::const_iterator it = documentObject.constBegin(); //
             it != documentObject.constEnd();                              //
             ++it) {
          _hashesToColors[it.key()] = it.value().toInt();
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
  QMap<QString, unsigned int>::const_iterator it = _hashesToColors.cbegin();
  while (it != _hashesToColors.end()) {
    documentObject.insert(it.key(), QJsonValue(int(it.value())));
    ++it;
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

QVector<TagColor> FiltersTagMap::usedColors(int * count)
{
  unsigned int all = 0;
  QMap<QString, unsigned int>::const_iterator it = _hashesToColors.cbegin();
  if (count) {
    memset(count, 0, sizeof(int) * int(TagColor::Count));
    while (it != _hashesToColors.cend()) {
      for (int iColor = 1 + (int)TagColor::None; iColor != (int)TagColor::Count; ++iColor) {
        count[iColor] += bool((1 << iColor) & it.value());
      }
      all |= it.value();
      ++it;
    }
  } else {
    while (it != _hashesToColors.end()) {
      all |= it.value();
      ++it;
    }
  }
  return uint2colors(all);
}

void FiltersTagMap::removeAllTags(TagColor color)
{
  QStringList toBeRemoved;
  QMap<QString, unsigned int>::iterator it = _hashesToColors.begin();
  unsigned int mask = ~(1 << int(color));
  while (it != _hashesToColors.end()) {
    it.value() = (it.value() & mask);
    if (!it.value()) {
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
  const unsigned int mask = ~(1 << int(color));
  it.value() = it.value() & mask;
  if (!it.value()) {
    _hashesToColors.erase(it);
  }
}

void FiltersTagMap::setFilterTag(const QString & hash, TagColor color)
{
  _hashesToColors[hash] = _hashesToColors[hash] | (1 << int(color));
}

void FiltersTagMap::toggleFilterTag(const QString & hash, TagColor color)
{
  _hashesToColors[hash] ^= (1 << int(color));
}

QVector<TagColor> FiltersTagMap::uint2colors(unsigned int value)
{
  QVector<TagColor> colors;
  for (unsigned int iColor = (int)TagColor::None + 1; iColor != (int)TagColor::Count; ++iColor) {
    if (value & (1 << iColor)) {
      colors.push_back(TagColor(iColor));
    }
  }
  return colors;
}

unsigned int FiltersTagMap::colors2uint(const QVector<TagColor> & colors)
{
  uint result = 0;
  for (const TagColor & color : colors) {
    result |= (1 << int(color));
  }
  return result;
}

} // namespace GmicQt
