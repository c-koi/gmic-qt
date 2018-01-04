/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file LanguageSelectionWidget.cpp
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
#include "LanguageSelectionWidget.h"
#include "ui_languageselectionwidget.h"
#include <QDebug>
#include <QLocale>
#include <QStringList>
#include <QSettings>
#include "Common.h"

LanguageSelectionWidget::LanguageSelectionWidget(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::LanguageSelectionWidget),
  _code2name(availableLanguages())
{
  ui->setupUi(this);
  QMap<QString,QString>::const_iterator it = _code2name.begin();
  while (it != _code2name.end()) {
    ui->comboBox->addItem(*it,QVariant(it.key()));
    ++it;
  }

  QString lang = systemDefaultAndAvailableLanguageCode();
  _systemDefaultIsAvailable = ! lang.isEmpty();
  if ( _systemDefaultIsAvailable ) {
    ui->comboBox->insertItem(0,QString(tr("System default (%1)")).arg(_code2name[lang]),QVariant(QString()));
  }
}

LanguageSelectionWidget::~LanguageSelectionWidget()
{
  delete ui;
}

QString LanguageSelectionWidget::selectedLanguageCode()
{
  return ui->comboBox->currentData().toString();
}

const QMap<QString, QString> & LanguageSelectionWidget::availableLanguages()
{
  static QMap<QString, QString> result;
  if ( result.isEmpty() ) {
    result["en"] = QString("English");
    result["cs"] = QString::fromUtf8("\xc4\x8c\x65\xc5\xa1\x74\x69\x6e\x61");
    result["de"] = QString("Deutsch");
    result["es"] = QString::fromUtf8("Espa\xc3\xb1ol");
    result["fr"] = QString::fromUtf8("Fran\xc3\xa7""ais");
    result["id"] = QString("bahasa Indonesia");
    result["it"] = QString("Italiano");
    result["ja"] = QString::fromUtf8("\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e");
    result["nl"] = QString("Dutch");
    result["pl"] = QString::fromUtf8("J\xc4\x99zyk polski");
    result["pt"] = QString::fromUtf8("Portugu\xc3\xaas");
    result["ru"] = QString::fromUtf8("\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9");
    result["ua"] = QString::fromUtf8("\xd0\xa3\xd0\xba\xd1\x80\xd0\xb0\xd1\x97\xd0\xbd\xd1\x81\xd1\x8c\xd0\xba\xd0\xb0");
    result["zh"] = QString::fromUtf8("\xe7\xae\x80\xe5\x8c\x96\xe5\xad\x97");
    result["zh_tw"] = QString::fromUtf8("\xe6\xad\xa3\xe9\xab\x94\xe5\xad\x97\x2f\xe7\xb9\x81\xe9\xab\x94\xe5\xad\x97");
  }
  return result;
}

QString LanguageSelectionWidget::configuredTranslator()
{
  QString code = QSettings().value("Config/LanguageCode",QString()).toString();
  if ( code.isEmpty() ) {
    code = systemDefaultAndAvailableLanguageCode();
    if ( code.isEmpty() ) {
      code = "en";
    }
  } else {
    QMap<QString,QString> map = availableLanguages();
    if ( map.find(code) == map.end() ) {
      code = "en";
    }
  }
  return code;
}

QString LanguageSelectionWidget::systemDefaultAndAvailableLanguageCode()
{
  QList<QString> languages = QLocale().uiLanguages();
  if ( languages.size() ) {
    QString lang = languages.front().split("-").front();
    QMap<QString,QString> map = availableLanguages();
    // Check for traditional Chinese (following https://stackoverflow.com/a/4894634)
    if ( ( lang == "zh" ) && ( languages.front().endsWith("TW") || languages.front().endsWith("HK") ) ) {
      return  QString("zh_tw");
    } else if ( map.find(lang) != map.end() ) {
      return lang;
    }
  }
  return QString();
}

void LanguageSelectionWidget::selectLanguage(const QString & code)
{
  int count = ui->comboBox->count();
  QString lang;
  // Empty code means "system default"
  if ( code.isEmpty() ) {
    if ( _systemDefaultIsAvailable ) {
      ui->comboBox->setCurrentIndex(0);
      return;
    } else {
      lang = "en";
    }
  } else {
    if ( _code2name.find(code) != _code2name.end() ) {
      lang = code;
    } else {
      lang = "en";
    }
  }
  for (int i = _systemDefaultIsAvailable?1:0; i < count; ++i) {
    if (ui->comboBox->itemData(i).toString() == lang) {
      ui->comboBox->setCurrentIndex(i);
      return;
    }
  }
}
