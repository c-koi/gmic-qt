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
#include <QFile>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "Common.h"
#include "ParametersCache.h"
#include "gmic.h"

QHash<QString,QList<QString>> ParametersCache::_parametersCache;
QHash<QString,InOutPanel::State> ParametersCache::_inOutPanelStates;

void ParametersCache::load(bool loadFiltersParameters)
{
  QString path = QString("%1%2").arg( GmicQt::path_rc(false), PARAMETERS_CACHE_FILENAME );
  QFile file(path);
  if ( file.open(QFile::ReadOnly) ) {
    QString line;
    do {
      line = file.readLine();
    } while ( file.bytesAvailable() && line != QString("[Filter parameters (compressed)]\n") );
    QByteArray data = qUncompress(file.readAll());
    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadOnly);

    QDataStream stream(&buffer);
    stream.setVersion(QDataStream::Qt_5_0);
    qint32 count;
    stream >> count;
    QString hash;
    QList<QString> values;
    while ( count-- ) {
      stream >> hash >> values;
      setValue(hash,values);
    }
  }

  // Load JSON file

  _parametersCache.clear();
  _inOutPanelStates.clear();

  QString jsonFilename = QString("%1%2").arg( GmicQt::path_rc(true), "gmic_qt_parameters_json.dat" );
  QFile jsonFile(jsonFilename);
  if ( jsonFile.open(QFile::ReadOnly) ) {
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
    // QJsonDocument jsonDoc = QJsonDocument::fromJson(qUncompress(jsonFile.readAll()));
    // QJsonDocument jsonDoc = QJsonDocument::fromBinaryData(qUncompress(jsonFile.readAll()));

    if ( jsonDoc.isObject() ) {
      QJsonObject documentObject = jsonDoc.object();
      QJsonObject::iterator itFilter = documentObject.begin();
      while ( itFilter != documentObject.end() ) {
        QString hash = itFilter.key();
        QJsonObject filterObject = itFilter.value().toObject();

        // Retrieve parameters
        if ( loadFiltersParameters ) {
          QJsonValue parameters = filterObject.value("parameters");
          if ( ! parameters.isUndefined() ) {
            QJsonArray array = parameters.toArray();
            QStringList values;
            for ( const QJsonValue & v : array ) {
              values.push_back(v.toString());
            }
            _parametersCache[hash] = values;
          }
        }

        QJsonValue state = filterObject.value("in_out_state");
        // Retrieve Input/Output state
        if ( ! state.isUndefined() ) {
          QJsonObject stateObject = state.toObject();
          _inOutPanelStates[hash] = InOutPanel::State::fromJSONObject(stateObject);
        }
        ++itFilter;
      }
    }
  } else {
    qWarning() << "[gmic-qt] Error: Cannot read" << jsonFilename;
    qWarning() << "[gmic-qt] Parameters cannot be restored.";
  }
}

void
ParametersCache::save()
{
  QByteArray data;
  QBuffer buffer(&data);
  buffer.open(QIODevice::WriteOnly);
  QDataStream stream(&buffer);
  stream.setVersion(QDataStream::Qt_5_0);
  qint32 count = _parametersCache.size();
  stream << count;
  QHash<QString,QList<QString>>::const_iterator it = _parametersCache.begin();
  while ( it != _parametersCache.end() ) {
    stream << it.key() << it.value();
    ++it;
  }

  QString path = QString("%1%2").arg( GmicQt::path_rc(true), PARAMETERS_CACHE_FILENAME );
  QFile file(path);
  if ( file.open(QFile::WriteOnly) ) {
    file.write(QString("Version=%1.%2.%3\n").arg(gmic_version/100).arg((gmic_version/10)%10).arg(gmic_version%10).toLocal8Bit());
    file.write(QString("[Filter parameters (compressed)]\n").toLocal8Bit());
    file.write(qCompress(data));
    file.close();
  } else {
    qWarning() << "[gmic-qt] Error: Cannot write" << path;
  }

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

  QHash<QString,InOutPanel::State>::iterator itState = _inOutPanelStates.begin();
  while ( itState != _inOutPanelStates.end() )  {
    QJsonObject filterObject;
    filterObject.insert("in_out_state",itState.value().toJSONObject());
    documentObject.insert(itState.key(),filterObject);
    ++itState;
  }

  // Add filters parameters

  QHash<QString,QList<QString> >::iterator itParams = _parametersCache.begin();
  while ( itParams != _parametersCache.end() ) {
    QJsonObject filterObject;
    QJsonObject::iterator entry = documentObject.find(itParams.key());
    if ( entry != documentObject.end() ) {
      filterObject = entry.value().toObject();
    }
    // Add the parameters list
    QJsonArray array;
    QStringList list = itParams.value();
    for ( const QString & str : list ) {
      array.push_back(str);
    }
    filterObject.insert("parameters",array);
    documentObject.insert(itParams.key(),filterObject);
    ++itParams;
  }

  QJsonDocument jsonDoc(documentObject);
  QString jsonFilename = QString("%1%2").arg( GmicQt::path_rc(true), "gmic_qt_parameters_json.dat" );
  QFile jsonFile(jsonFilename);
  if ( jsonFile.open(QFile::WriteOnly|QFile::Truncate) ) {
    //jsonFile.write(jsonDoc.toBinaryData());
    jsonFile.write(jsonDoc.toJson());
    //jsonFile.write(qCompress(jsonDoc.toBinaryData()));
    jsonFile.close();
  } else {
    qWarning() << "[gmic-qt] Error: Cannot write" << jsonFilename;
    qWarning() << "[gmic-qt] Parameters cannot be saved.";
  }
}

void
ParametersCache::setValue(const QString & hash, const QList<QString> & list)
{
  _parametersCache[hash] = list;
}

QList<QString>
ParametersCache::getValue(const QString & hash)
{
  if ( _parametersCache.count(hash) ) {
    return _parametersCache[hash];
  } else {
    return QList<QString>();
  }
}

void
ParametersCache::remove(const QString & hash)
{
  _parametersCache.remove(hash);
  _inOutPanelStates.remove(hash);
}

InOutPanel::State ParametersCache::getInputOutputState(const QString & hash)
{
  if ( _inOutPanelStates.contains(hash) ) {
    return _inOutPanelStates[hash];
  } else {
    return InOutPanel::State::Unspecified;
  }
}

void ParametersCache::setInputOutputState(const QString & hash, const InOutPanel::State & state)
{
  if ( state.isUnspecified() ) {
    _inOutPanelStates.remove(hash);
    return;
  }
  _inOutPanelStates[hash] = state;
}
