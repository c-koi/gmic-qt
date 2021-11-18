/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file LanguageSettings.cpp
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

#include "LanguageSettings.h"
#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>
#include "Common.h"
#include "Logger.h"

namespace GmicQt
{

const QMap<QString, QString> & LanguageSettings::availableLanguages()
{
  static QMap<QString, QString> result;
  if (result.isEmpty()) {
    result["en"] = QString("English");
    result["cs"] = QString::fromUtf8("\xc4\x8c\x65\xc5\xa1\x74\x69\x6e\x61");
    result["de"] = QString("Deutsch");
    result["es"] = QString::fromUtf8("Espa\xc3\xb1ol");
    result["fr"] = QString::fromUtf8("Fran\xc3\xa7"
                                     "ais");
    result["id"] = QString("bahasa Indonesia");
    result["it"] = QString("Italiano");
    result["ja"] = QString::fromUtf8("\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e");
    result["nl"] = QString("Dutch");
    result["pl"] = QString::fromUtf8("J\xc4\x99zyk polski");
    result["pt"] = QString::fromUtf8("Portugu\xc3\xaas");
    result["ru"] = QString::fromUtf8("\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9");
    result["sv"] = QString::fromUtf8("Svenska");
    result["ua"] = QString::fromUtf8("\xd0\xa3\xd0\xba\xd1\x80\xd0\xb0\xd1\x97\xd0\xbd\xd1\x81\xd1\x8c\xd0\xba\xd0\xb0");
    result["zh"] = QString::fromUtf8("\xe7\xae\x80\xe5\x8c\x96\xe5\xad\x97");
    result["zh_tw"] = QString::fromUtf8("\xe6\xad\xa3\xe9\xab\x94\xe5\xad\x97\x2f\xe7\xb9\x81\xe9\xab\x94\xe5\xad\x97");
  }
  return result;
}

QString LanguageSettings::configuredTranslator()
{
  QString code = QSettings().value("Config/LanguageCode", QString()).toString();
  if (code.isEmpty()) {
    code = systemDefaultAndAvailableLanguageCode();
    if (code.isEmpty()) {
      code = "en";
    }
  } else {
    QMap<QString, QString> map = availableLanguages();
    if (map.find(code) == map.end()) {
      code = "en";
    }
  }
  return code;
}

QString LanguageSettings::systemDefaultAndAvailableLanguageCode()
{
  QList<QString> languages = QLocale().uiLanguages();
  if (!languages.isEmpty()) {
    QString lang = languages.front().split("-").front();
    QMap<QString, QString> map = availableLanguages();
    // Check for traditional Chinese (following https://stackoverflow.com/a/4894634)
    if ((lang == "zh") && (languages.front().endsWith("TW") || languages.front().endsWith("HK"))) {
      return QString("zh_tw");
    }
    if (map.find(lang) != map.end()) {
      return lang;
    }
  }
  return QString();
}

// Translate according to current locale or configured language
void LanguageSettings::installTranslators()
{
  QString lang = LanguageSettings::configuredTranslator();
  if (!lang.isEmpty() && (lang != "en")) {
    installQtTranslator(lang);
    installTranslator(QString(":/translations/%1.qm").arg(lang));
    installTranslator(QString(":/translations/filters/%1.qm").arg(lang));
  }
}

void LanguageSettings::installTranslator(const QString & qmPath)
{
  if (!QFileInfo(qmPath).isReadable()) {
    return;
  }
  auto translator = new QTranslator(qApp);
  if (translator->load(qmPath)) {
    if (!QApplication::installTranslator(translator)) {
      Logger::error(QObject::tr("Could not install translator for file %1").arg(qmPath));
    }
  } else {
    Logger::error(QObject::tr("Could not load filter translation file %1").arg(qmPath));
    translator->deleteLater();
  }
}

void LanguageSettings::installQtTranslator(const QString & lang)
{
  auto qtTranslator = new QTranslator(qApp);
  if (qtTranslator->load(QString("qt_%1").arg(lang), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
    QApplication::installTranslator(qtTranslator);
  } else {
    qtTranslator->deleteLater();
  }
}

} // namespace GmicQt
