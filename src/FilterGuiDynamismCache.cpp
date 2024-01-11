/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterGuiDynamismCache.cpp
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
#include "FilterGuiDynamismCache.h"
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>
#include "Common.h"
#include "Globals.h"
#include "Logger.h"
#include "Utils.h"
#include "gmic.h"

namespace GmicQt
{
QHash<QString, int> FilterGuiDynamismCache::_dynamismCache;

void FilterGuiDynamismCache::load()
{
  _dynamismCache.clear();
  QString jsonFilename = QString("%1%2").arg(gmicConfigPath(true), FILTER_GUI_DYNAMISM_CACHE_FILENAME);
  QFile jsonFile(jsonFilename);
  if (!jsonFile.exists()) {
    return;
  }
  if (jsonFile.open(QFile::ReadOnly)) {
    QJsonDocument jsonDoc;
    QByteArray allFile = jsonFile.readAll();
    if (allFile.startsWith("{")) { // Was created in debug mode
      jsonDoc = QJsonDocument::fromJson(allFile);
    } else {
      jsonDoc = QJsonDocument::fromJson(qUncompress(allFile));
    }
    if (jsonDoc.isNull()) {
      Logger::warning(QString("Cannot parse ") + jsonFilename);
      Logger::warning("Last filters parameters are lost!");
    } else {
      if (!jsonDoc.isObject()) {
        Logger::error(QString("JSON file format is not correct (") + jsonFilename + ")");
      } else {
        QJsonObject documentObject = jsonDoc.object();
        QJsonObject::iterator itFilter = documentObject.begin();
        while (itFilter != documentObject.end()) {
          QString hash = itFilter.key();
          QString status = itFilter.value().toString();
          if (status == "Static") {
            _dynamismCache.insert(hash, FilterGuiDynamism::Static);
          } else if (status == "Dynamic") {
            _dynamismCache.insert(hash, FilterGuiDynamism::Dynamic);
          }
          ++itFilter;
        }
      }
    }
  } else {
    Logger::error("Cannot open " + jsonFilename);
    Logger::error("Parameters cannot be restored");
  }
}

void FilterGuiDynamismCache::save()
{
  // JSON Document format
  //
  // {
  //  "51d288e6f1c6e531cc61289f17e34d8a": {
  //      "parameters": [
  //          "6",
  //          "21.06",
  //          "1.36",
  //          "5",
  //          "0"
  //      ],
  //      "in_out_state": {
  //          "InputLayers": 1,
  //          "OutputMessages": 5,
  //          "OutputMode": 100,
  //          "PreviewMode": 100
  //      }
  //      "visibility_states": [
  //             0,
  //             1,
  //             2,
  //             0
  //      ]
  //  }
  // }

  QJsonObject documentObject;
  QHash<QString, int>::iterator it = _dynamismCache.begin();
  while (it != _dynamismCache.end()) {
    if (it.value() == FilterGuiDynamism::Unknown) {
      ++it;
      continue;
    }
    QJsonValue status((it.value() == FilterGuiDynamism::Static) ? "Static" : "Dynamic");
    documentObject.insert(it.key(), status);
    ++it;
  }

  QJsonDocument jsonDoc(documentObject);
  QString jsonFilename = QString("%1%2").arg(gmicConfigPath(true), FILTER_GUI_DYNAMISM_CACHE_FILENAME);
#ifdef _GMIC_QT_DEBUG_
  QByteArray array(jsonDoc.toJson());
#else
  QByteArray array(qCompress(jsonDoc.toJson(QJsonDocument::Compact)));
#endif
  if (!safelyWrite(array, jsonFilename)) {
    Logger::error("Cannot write " + jsonFilename);
    Logger::error("Parameters cannot be saved");
  }
}

void FilterGuiDynamismCache::setValue(const QString & hash, FilterGuiDynamism dynamism)
{
  _dynamismCache.insert(hash, int(dynamism));
}

FilterGuiDynamism FilterGuiDynamismCache::getValue(const QString & hash)
{
  auto it = _dynamismCache.find(hash);
  if (it != _dynamismCache.end()) {
    return FilterGuiDynamism(it.value());
  }
  return FilterGuiDynamism::Unknown;
}

void FilterGuiDynamismCache::remove(const QString & hash)
{
  _dynamismCache.remove(hash);
}

void FilterGuiDynamismCache::clear()
{
  _dynamismCache.clear();
}

} // namespace GmicQt
