/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FavesModelReader.cpp
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
#include "FilterSelector/FavesModelReader.h"
#include <QBuffer>
#include <QDebug>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QList>
#include <QLocale>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include "Common.h"
#include "FilterSelector/FavesModel.h"
#include "Utils.h"
#include "gmic.h"

FavesModelReader::FavesModelReader(FavesModel & model) : _model(model) {}

bool FavesModelReader::gmicGTKFaveFileAvailable()
{
  QFileInfo info(gmicGTKFavesFilename());
  return info.isReadable();
}

FavesModel::Fave FavesModelReader::jsonObjectToFave(const QJsonObject & object)
{
  FavesModel::Fave fave;
  fave.setName(object.value("Name").toString(""));
  fave.setOriginalName(object.value("originalName").toString(""));
  fave.setCommand(object.value("command").toString(""));
  fave.setPreviewCommand(object.value("preview").toString());
  QStringList defaultParameters;
  QJsonArray array = object.value("defaultParameters").toArray();
  for (const QJsonValueRef & value : array) {
    defaultParameters.push_back(value.toString());
  }
  fave.setDefaultValues(defaultParameters);
  QList<int> defaultVisibilities;
  array = object.value("defaultVisibilities").toArray();
  for (const QJsonValueRef & value : array) {
    defaultVisibilities.push_back(value.toInt());
  }
  fave.setDefaultVisibilities(defaultVisibilities);
  fave.build();
  return fave;
}

void FavesModelReader::importFavesFromGmicGTK()
{
  QString filename = gmicGTKFavesFilename();
  QFile file(filename);
  if (file.open(QIODevice::ReadOnly)) {
    QString line;
    int lineNumber = 1;
    while (!(line = file.readLine()).isEmpty()) {
      line = line.trimmed();
      line.replace(QRegExp("^."), "").replace(QRegExp(".$"), "");
      QList<QString> list = line.split("}{");
      for (QString & str : list) {
        str.replace(QChar(gmic_lbrace), QString("{"));
        str.replace(QChar(gmic_rbrace), QString("}"));
      }
      if (list.size() >= 4) {
        FavesModel::Fave fave;
        fave.setName(list.front());
        fave.setOriginalName(list[1]);
        fave.setCommand(list[2].replace(QRegularExpression("^gimp_"), "fx_"));
        fave.setPreviewCommand(list[3].replace(QRegularExpression("^gimp_"), "fx_"));
        list.pop_front();
        list.pop_front();
        list.pop_front();
        list.pop_front();
        fave.setDefaultValues(list);
        fave.build();
        _model.addFave(fave);
      } else {
        std::cerr << "[gmic-qt] Error: Import failed for fave at gimp_faves:" << lineNumber << "\n";
      }
      ++lineNumber;
    }
  } else {
    qWarning() << "[gmic-qt] Error: Import failed. Cannot open" << filename;
  }
}

void FavesModelReader::loadFaves()
{
  // Read JSON faves if file exists
  QString jsonFilename(QString("%1%2").arg(GmicQt::path_rc(false)).arg("gmic_qt_faves.json"));
  QFile jsonFile(jsonFilename);
  if (jsonFile.exists()) {
    if (jsonFile.open(QIODevice::ReadOnly)) {
      QJsonDocument document;
      QJsonParseError parseError;
      document = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
      if (parseError.error == QJsonParseError::NoError) {
        QJsonArray array = document.array();
        for (const QJsonValueRef & value : array) {
          _model.addFave(jsonObjectToFave(value.toObject()));
        }
      } else {
        qWarning() << "[gmic-qt] Error loading faves (parse error) : " << jsonFilename;
        qWarning() << "[gmic-qt]" << parseError.errorString();
      }
    } else {
      qWarning() << "[gmic-qt] Error: Faves loading failed: Cannot open" << jsonFilename;
    }
    return;
  }

  // Read old 2.0.0 prerelease file format if no JSON was found
  QString filename(QString("%1%2").arg(GmicQt::path_rc(false)).arg("gmic_qt_faves"));
  QFile file(filename);
  if (file.exists()) {
    if (file.open(QIODevice::ReadOnly)) {
      QString line;
      int lineNumber = 1;
      while (!(line = file.readLine()).isEmpty()) {
        line = line.trimmed();
        if (line.startsWith("{")) {
          line.replace(QRegExp("^."), "").replace(QRegExp(".$"), "");
          QList<QString> list = line.split("}{");
          for (QString & str : list) {
            str.replace(QChar(gmic_lbrace), QString("{"));
            str.replace(QChar(gmic_rbrace), QString("}"));
            str.replace(QChar(gmic_newline), QString("\n"));
          }
          if (list.size() >= 4) {
            FavesModel::Fave fave;
            fave.setName(list.front());
            fave.setOriginalName(list[1]);
            fave.setCommand(list[2]);
            fave.setPreviewCommand(list[3]);
            list.pop_front();
            list.pop_front();
            list.pop_front();
            list.pop_front();
            fave.setDefaultValues(list);
            fave.build();
            _model.addFave(fave);
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
}

QString FavesModelReader::gmicGTKFavesFilename()
{
  return QString("%1%2").arg(GmicQt::path_rc(false)).arg("gimp_faves");
}
