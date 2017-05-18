/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file StoredFave.cpp
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
#include "StoredFave.h"
#include "FiltersTreeAbstractFilterItem.h"
#include <QFileInfo>
#include <QSettings>
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QDebug>
#include "FiltersTreeFaveItem.h"
#include "Common.h"
#include "gmic_qt.h"
#include "gmic.h"

StoredFave::StoredFave(QString name,
                       QString originalName,
                       QString command,
                       QString previewCommand,
                       QStringList defaultParameters)
  : _name(name),
    _originalName(originalName),
    _command(command),
    _previewCommand(previewCommand),
    _defaultParameters(defaultParameters)
{
  _inputMode = GmicQt::DefaultInputMode;
  _outputMode = GmicQt::DefaultOutputMode;
  _outputMessageMode = GmicQt::DefaultOutputMessageMode;
  _previewMode = GmicQt::DefaultPreviewMode;
}

StoredFave::StoredFave(QString name,
                       QString originalName,
                       QString command,
                       QString previewCommand,
                       QStringList defaultParameters,
                       GmicQt::InputMode inputMode,
                       GmicQt::OutputMode outputMode,
                       GmicQt::OutputMessageMode outputMessageMode,
                       GmicQt::PreviewMode previewMode)
  : _name(name),
    _originalName(originalName),
    _command(command),
    _previewCommand(previewCommand),
    _defaultParameters(defaultParameters),
    _inputMode(inputMode),
    _outputMode(outputMode),
    _outputMessageMode(outputMessageMode),
    _previewMode(previewMode)
{
}

StoredFave::StoredFave(const FiltersTreeFaveItem * fave)
  : _name(fave->name()),
    _originalName(fave->originalFilterName()),
    _command(fave->command()),
    _previewCommand(fave->previewCommand()),
    _defaultParameters(fave->defaultValues()),
    _inputMode(fave->inputMode()),
    _outputMode(fave->outputMode()),
    _outputMessageMode(fave->outputMessageMode()),
    _previewMode(fave->previewMode())
{
}

QString StoredFave::originalFilterHash()
{
  return FiltersTreeAbstractFilterItem::computeHash(_originalName,_command,_previewCommand);
}

QTextStream & StoredFave::flush(QTextStream & stream) const
{
  QList<QString> list;
  list.append(_name);
  list.append(_originalName);
  list.append(_command);
  list.append(_previewCommand);
  list.append(_defaultParameters);
  for ( QString & str : list ) {
    str.replace(QString("{"),QChar(gmic_lbrace));
    str.replace(QString("}"),QChar(gmic_rbrace));
    str.replace(QChar(10),QChar(gmic_newline));
    str.replace(QChar(13),"");
    stream << "{" << str << "}";
  }
  stream << "\n";
  return stream;
}

QTextStream & operator<<(QTextStream & stream, const StoredFave & fave)
{
  return fave.flush(stream);
}

QList<StoredFave> StoredFave::importFaves()
{
  QList<StoredFave> faves;
  QString filename = QString("%1%2").arg(GmicQt::path_rc(false)).arg("gimp_faves");
  QFile file(filename);
  if  ( file.open(QIODevice::ReadOnly) ) {
    QString line;
    int lineNumber = 1;
    while ( ! (line = file.readLine()).isEmpty() ) {

      line = line.trimmed();
      line.replace(QRegExp("^."),"").replace(QRegExp(".$"),"");
      QList<QString> list = line.split("}{");
      for ( QString & str : list ) {
        str.replace(QChar(gmic_lbrace),QString("{"));
        str.replace(QChar(gmic_rbrace),QString("}"));
      }
      if ( list.size() >= 4 ) {
        QString name = list.front();
        QString originalName = list[1];
        QString command = list[2];
        command.replace(QRegularExpression("^gimp_"),"fx_");
        QString previewCommand = list[3];
        previewCommand.replace(QRegularExpression("^gimp_"),"fx_");
        list.pop_front(); list.pop_front(); list.pop_front(); list.pop_front();
        faves.push_back(StoredFave(name,originalName,command,previewCommand,list));
      } else {
        std::cerr << "[gmic-qt] Error: Import failed for fave at gimp_faves:" << lineNumber << "\n";
      }
      ++lineNumber;
    }
  } else {
    qWarning() << "[gmic-qt] Error: Import failed. Cannot open" << filename;
  }
  return faves;
}

QList<StoredFave> StoredFave::readFaves()
{
  QList<StoredFave> faves;

  // Read JSON faves if file exists
  QString jsonFilename(QString("%1%2").arg(GmicQt::path_rc(false)).arg("gmic_qt_faves.json"));
  QFile jsonFile(jsonFilename);
  if ( jsonFile.exists() ) {
    if ( jsonFile.open(QIODevice::ReadOnly) ) {
      QJsonDocument document;
      QJsonParseError parseError;
      document = QJsonDocument::fromJson(jsonFile.readAll(),&parseError);
      if ( parseError.error == QJsonParseError::NoError ) {
        QJsonArray array = document.array();
        for ( const QJsonValue & value : array ) {
          faves.push_back(StoredFave::fromJSONObject(value.toObject()));
        }
      } else {
        qWarning() << "[gmic-qt] Error loading faves (parse error) : " << jsonFilename;
        qWarning() << "[gmic-qt]" << parseError.errorString();
      }
    } else {
      qWarning() << "[gmic-qt] Error: Faves loading failed: Cannot open" << jsonFilename;
    }
    return faves;
  }

  // Read old 2.0.0 prerelease file format if no JSON was found
  QString filename(QString("%1%2").arg(GmicQt::path_rc(false)).arg("gmic_qt_faves"));
  QFile file(filename);
  if ( file.exists() ) {
    if  ( file.open(QIODevice::ReadOnly) ) {
      QString line;
      int lineNumber = 1;
      while ( ! (line = file.readLine()).isEmpty() ) {
        line = line.trimmed();
        if ( line.startsWith("{") ) {
          line.replace(QRegExp("^."),"").replace(QRegExp(".$"),"");
          QList<QString> list = line.split("}{");
          for ( QString & str : list ) {
            str.replace(QChar(gmic_lbrace),QString("{"));
            str.replace(QChar(gmic_rbrace),QString("}"));
            str.replace(QChar(gmic_newline),QString("\n"));
          }
          if ( list.size() >= 4 ) {
            QString name = list.front();
            QString originalName = list[1];
            QString command = list[2];
            QString previewCommand = list[3];
            list.pop_front(); list.pop_front(); list.pop_front(); list.pop_front();
            faves.push_back(StoredFave(name,originalName,command,previewCommand,list));
          } else {
            std::cerr << "[gmic-qt] Error: Loading failed for fave at gmic_qt_faves:" << lineNumber << "\n";
          }
        }
        ++lineNumber;
      }
    } else {
      qWarning() << "[gmic-qt] Error: Loading failed. Cannot open" << filename;
    }
  }
  return faves;
}

QJsonObject StoredFave::toJSONObject() const
{
  QJsonObject object;
  object["Name"] = _name;
  object["originalName"] =_originalName;
  object["command"] =_command;
  object["preview"] = _previewCommand;
  QJsonArray array;
  for ( const QString & str : _defaultParameters ) {
    array.push_back(str);
  }
  object["defaultParameters"] = array;

  object["inputMode"] = _inputMode;
  object["outputMode"] = _outputMode;
  object["outputMessageMode"] = _outputMessageMode;
  object["previewMode"] = _previewMode;
  return object;
}

StoredFave StoredFave::fromJSONObject(const QJsonObject & object)
{
  QString name = object.value("Name").toString("");
  QString originalName = object.value("originalName").toString("");
  QString command = object.value("command").toString("");
  QString previewCommand = object.value("preview").toString();

  GmicQt::InputMode inputMode = static_cast<GmicQt::InputMode>(object.value("inputMode").toInt(GmicQt::DefaultInputMode));
  GmicQt::OutputMode outputMode = static_cast<GmicQt::OutputMode>(object.value("outputMode").toInt(GmicQt::DefaultOutputMode));
  GmicQt::OutputMessageMode outputMessageMode = static_cast<GmicQt::OutputMessageMode>(object.value("outputMessageMode").toInt(GmicQt::DefaultOutputMessageMode));
  GmicQt::PreviewMode previewMode = static_cast<GmicQt::PreviewMode>(object.value("previewMode").toInt(GmicQt::DefaultPreviewMode));

  QStringList defaultParameters;
  QJsonArray array = object.value("defaultParameters").toArray();
  for ( const QJsonValue & value : array ) {
    defaultParameters.push_back(value.toString());
  }
  return StoredFave(name,originalName,command,previewCommand,defaultParameters,inputMode,outputMode,outputMessageMode,previewMode);
}
