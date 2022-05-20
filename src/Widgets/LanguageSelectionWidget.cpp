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
#include "Widgets/LanguageSelectionWidget.h"
#include <QDebug>
#include <QSettings>
#include <QStringList>
#include "Common.h"
#include "Globals.h"
#include "LanguageSettings.h"
#include "Settings.h"
#include "ui_languageselectionwidget.h"

namespace GmicQt
{

LanguageSelectionWidget::LanguageSelectionWidget(QWidget * parent) //
    : QWidget(parent),                                             //
      ui(new Ui::LanguageSelectionWidget),                         //
      _code2name(LanguageSettings::availableLanguages())
{
  ui->setupUi(this);
  QMap<QString, QString>::const_iterator it = _code2name.begin();
  while (it != _code2name.end()) {
    ui->comboBox->addItem(*it, QVariant(it.key()));
    ++it;
  }

  QString lang = LanguageSettings::systemDefaultAndAvailableLanguageCode();
  _systemDefaultIsAvailable = !lang.isEmpty();
  if (_systemDefaultIsAvailable) {
    ui->comboBox->insertItem(0, QString(tr("System default (%1)")).arg(_code2name[lang]), QVariant(QString()));
  }

  if (Settings::darkThemeEnabled()) {
    QPalette p = ui->cbTranslateFilters->palette();
    p.setColor(QPalette::Text, Settings::CheckBoxTextColor);
    p.setColor(QPalette::Base, Settings::CheckBoxBaseColor);
    ui->cbTranslateFilters->setPalette(p);
  }
  ui->cbTranslateFilters->setToolTip(tr("Translations are very likely to be incomplete."));

  connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LanguageSelectionWidget::onLanguageSelectionChanged);
  connect(ui->cbTranslateFilters, &QCheckBox::toggled, this, &LanguageSelectionWidget::onCheckboxToggled);
}

LanguageSelectionWidget::~LanguageSelectionWidget()
{
  delete ui;
}

QString LanguageSelectionWidget::selectedLanguageCode() const
{
  return ui->comboBox->currentData().toString();
}

bool LanguageSelectionWidget::translateFiltersEnabled() const
{
  return ui->cbTranslateFilters->isChecked();
}

void LanguageSelectionWidget::enableFilterTranslation(bool on)
{
  ui->cbTranslateFilters->setChecked(on);
}

void LanguageSelectionWidget::selectLanguage(const QString & code)
{
  int count = ui->comboBox->count();
  QString lang;
  // Empty code means "system default"
  if (code.isEmpty()) {
    if (_systemDefaultIsAvailable) {
      ui->comboBox->setCurrentIndex(0);
      return;
    }
    lang = "en";
  } else {
    if (_code2name.find(code) != _code2name.end()) {
      lang = code;
    } else {
      lang = "en";
    }
  }
  for (int i = _systemDefaultIsAvailable ? 1 : 0; i < count; ++i) {
    if (ui->comboBox->itemData(i).toString() == lang) {
      ui->comboBox->setCurrentIndex(i);
      return;
    }
  }
}

void LanguageSelectionWidget::onLanguageSelectionChanged(int index)
{
  QString lang = ui->comboBox->itemData(index).toString();
  if (lang.isEmpty()) {
    lang = LanguageSettings::systemDefaultAndAvailableLanguageCode();
  }
  if (LanguageSettings::filterTranslationAvailable(lang)) {
    ui->cbTranslateFilters->setEnabled(true);
  } else {
    ui->cbTranslateFilters->setChecked(false);
    ui->cbTranslateFilters->setEnabled(false);
  }
  emit languageCodeSelected(lang);
  Settings::setLanguageCode(lang);
}

void LanguageSelectionWidget::onCheckboxToggled(bool on)
{
  Settings::setFilterTranslationEnabled(on);
}

} // namespace GmicQt
