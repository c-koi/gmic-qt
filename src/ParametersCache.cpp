/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ParametersCache.cpp
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
#include "ParametersCache.h"
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "Common.h"
#include "gmic.h"

QHash<QString, QList<QString>> ParametersCache::_parametersCache;
QHash<QString, InOutPanel::State> ParametersCache::_inOutPanelStates;

void ParametersCache::load(bool loadFiltersParameters)
{
  // Load JSON file
  _parametersCache.clear();
  _inOutPanelStates.clear();

  QString jsonFilename = QString("%1%2").arg(GmicQt::path_rc(true), PARAMETERS_CACHE_FILENAME);
  QFile jsonFile(jsonFilename);
  if (!jsonFile.exists()) {
    return;
  }
  if (jsonFile.open(QFile::ReadOnly)) {
    QJsonDocument jsonDoc = QJsonDocument::fromBinaryData(qUncompress(jsonFile.readAll()));
    if (jsonDoc.isNull()) {
      std::cerr << "[gmic-qt] Warning: cannot parse " << jsonFilename.toStdString() << std::endl;
      std::cerr << "[gmic-qt] Last filters parameters are lost!\n";
    } else {
      if (!jsonDoc.isObject()) {
        std::cerr << "[gmic-qt] Error: JSON file format is not correct (" << jsonFilename.toStdString() << ")\n";
      } else {
        QJsonObject documentObject = jsonDoc.object();
        QJsonObject::iterator itFilter = documentObject.begin();
        while (itFilter != documentObject.end()) {
          QString hash = itFilter.key();
          QJsonObject filterObject = itFilter.value().toObject();
          // Retrieve parameters
          if (loadFiltersParameters) {
            QJsonValue parameters = filterObject.value("parameters");
            if (!parameters.isUndefined()) {
              QJsonArray array = parameters.toArray();
              QStringList values;
              for (const QJsonValue & v : array) {
                values.push_back(v.toString());
              }
              _parametersCache[hash] = values;
            }
          }
          QJsonValue state = filterObject.value("in_out_state");
          // Retrieve Input/Output state
          if (!state.isUndefined()) {
            QJsonObject stateObject = state.toObject();
            _inOutPanelStates[hash] = InOutPanel::State::fromJSONObject(stateObject);
          }
          ++itFilter;
        }
      }
    }
  } else {
    std::cerr << "[gmic-qt] Error: Cannot read " << jsonFilename.toStdString() << std::endl;
    std::cerr << "[gmic-qt] Parameters cannot be restored.\n";
  }
}

void ParametersCache::save()
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
  //  }
  // }

  QJsonObject documentObject;

  // Add Input/Ouput states

  QHash<QString, InOutPanel::State>::iterator itState = _inOutPanelStates.begin();
  while (itState != _inOutPanelStates.end()) {
    QJsonObject filterObject;
    filterObject.insert("in_out_state", itState.value().toJSONObject());
    documentObject.insert(itState.key(), filterObject);
    ++itState;
  }

  // Add filters parameters

  QHash<QString, QList<QString>>::iterator itParams = _parametersCache.begin();
  while (itParams != _parametersCache.end()) {
    QJsonObject filterObject;
    QJsonObject::iterator entry = documentObject.find(itParams.key());
    if (entry != documentObject.end()) {
      filterObject = entry.value().toObject();
    }
    // Add the parameters list
    QJsonArray array;
    QStringList list = itParams.value();
    for (const QString & str : list) {
      array.push_back(str);
    }
    filterObject.insert("parameters", array);
    documentObject.insert(itParams.key(), filterObject);
    ++itParams;
  }

  QJsonDocument jsonDoc(documentObject);
  QString jsonFilename = QString("%1%2").arg(GmicQt::path_rc(true), PARAMETERS_CACHE_FILENAME);
  QFile jsonFile(jsonFilename);
  if (QFile::exists(jsonFilename)) {
    QString bakFilename = QString("%1%2").arg(GmicQt::path_rc(false), PARAMETERS_CACHE_FILENAME ".bak");
    QFile::remove(bakFilename);
    QFile::copy(jsonFilename, bakFilename);
  }
  if (jsonFile.open(QFile::WriteOnly | QFile::Truncate)) {
    qint64 count = jsonFile.write(qCompress(jsonDoc.toBinaryData()));
    // jsonFile.write(jsonDoc.toJson());
    // jsonFile.write(qCompress(jsonDoc.toBinaryData()));
    jsonFile.close();
    if (count != -1) {
      // Remove obsolete 2.0.0 pre-release files
      QString path = GmicQt::path_rc(true);
      QFile::remove(path + "gmic_qt_parameters.dat");
      QFile::remove(path + "gmic_qt_parameters.json");
      QFile::remove(path + "gmic_qt_parameters.json.bak");
      QFile::remove(path + "gmic_qt_parameters_json.dat");
    }
  } else {
    std::cerr << "[gmic-qt] Error: Cannot write " << jsonFilename.toStdString() << std::endl;
    std::cerr << "[gmic-qt] Parameters cannot be saved.\n";
  }
}

void ParametersCache::setValues(const QString & hash, const QList<QString> & values)
{
  _parametersCache[hash] = values;
}

QList<QString> ParametersCache::getValues(const QString & hash)
{
  if (_parametersCache.count(hash)) {
    return _parametersCache[hash];
  } else {
    return QList<QString>();
  }
}

void ParametersCache::remove(const QString & hash)
{
  _parametersCache.remove(hash);
  _inOutPanelStates.remove(hash);
}

InOutPanel::State ParametersCache::getInputOutputState(const QString & hash)
{
  if (_inOutPanelStates.contains(hash)) {
    return _inOutPanelStates[hash];
  } else {
    return InOutPanel::State::Unspecified;
  }
}

void ParametersCache::setInputOutputState(const QString & hash, const InOutPanel::State & state)
{
  if (state.isUnspecified()) {
    _inOutPanelStates.remove(hash);
    return;
  }
  _inOutPanelStates[hash] = state;
}

void ParametersCache::cleanup(const QSet<QString> & hashesToKeep)
{
  QSet<QString> obsoleteHashes;

  // Build set of no longer used parameters
  QHash<QString, QList<QString>>::iterator itParam = _parametersCache.begin();
  while (itParam != _parametersCache.end()) {
    if (!hashesToKeep.contains(itParam.key())) {
      obsoleteHashes.insert(itParam.key());
    }
    ++itParam;
  }
  for (const QString & h : obsoleteHashes) {
    _parametersCache.remove(h);
  }
  obsoleteHashes.clear();

  // Build set of no longer used In/Out states
  QHash<QString, InOutPanel::State>::iterator itState = _inOutPanelStates.begin();
  while (itState != _inOutPanelStates.end()) {
    if (!hashesToKeep.contains(itState.key())) {
      obsoleteHashes.insert(itState.key());
    }
    ++itState;
  }
  for (const QString & h : obsoleteHashes) {
    _inOutPanelStates.remove(h);
  }
  obsoleteHashes.clear();
}
