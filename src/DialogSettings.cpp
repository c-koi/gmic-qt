/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file DialogSettings.cpp
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
#include "DialogSettings.h"
#include <QCloseEvent>
#include <QSettings>
#include <limits>
#include "Common.h"
#include "Globals.h"
#include "IconLoader.h"
#include "Logger.h"
#include "Updater.h"
#include "ui_dialogsettings.h"

bool DialogSettings::_darkThemeEnabled;
QString DialogSettings::_languageCode;
bool DialogSettings::_nativeColorDialogs;
bool DialogSettings::_logosAreVisible;
MainWindow::PreviewPosition DialogSettings::_previewPosition;
int DialogSettings::_updatePeriodicity;
GmicQt::OutputMessageMode DialogSettings::_outputMessageMode;
bool DialogSettings::_previewZoomAlwaysEnabled = false;

const QColor DialogSettings::CheckBoxBaseColor(83, 83, 83);
const QColor DialogSettings::CheckBoxTextColor(255, 255, 255);
QColor DialogSettings::UnselectedFilterTextColor;

QString DialogSettings::FolderParameterDefaultValue;
QString DialogSettings::FileParameterDefaultPath;
int DialogSettings::_previewTimeout = 16;

QIcon DialogSettings::AddIcon;
QIcon DialogSettings::RemoveIcon;

// TODO : Make DialogSetting a view of a Settings class

DialogSettings::DialogSettings(QWidget * parent) : QDialog(parent), ui(new Ui::DialogSettings)
{
  ui->setupUi(this);

  setWindowTitle(tr("Settings"));
  setWindowIcon(parent->windowIcon());
  adjustSize();

  ui->pbUpdate->setIcon(LOAD_ICON("view-refresh"));

  ui->cbUpdatePeriodicity->addItem(tr("Never"), QVariant(INTERNET_NEVER_UPDATE_PERIODICITY));
  ui->cbUpdatePeriodicity->addItem(tr("Daily"), QVariant(24));
  ui->cbUpdatePeriodicity->addItem(tr("Weekly"), QVariant(7 * 24));
  ui->cbUpdatePeriodicity->addItem(tr("Every 2 weeks"), QVariant(14 * 24));
  ui->cbUpdatePeriodicity->addItem(tr("Monthly"), QVariant(30 * 24));
#ifdef _GMIC_QT_DEBUG_
  ui->cbUpdatePeriodicity->addItem(tr("At launch (debug)"), QVariant(0));
#endif
  for (int i = 0; i < ui->cbUpdatePeriodicity->count(); ++i) {
    if (_updatePeriodicity == ui->cbUpdatePeriodicity->itemData(i).toInt()) {
      ui->cbUpdatePeriodicity->setCurrentIndex(i);
    }
  }

  ui->outputMessages->setToolTip(tr("Output messages"));
  QString dummy3(tr("Output messages..."));
  ui->outputMessages->addItem(tr("Quiet (default)"), GmicQt::Quiet);
  ui->outputMessages->addItem(tr("Verbose (layer name)"), GmicQt::VerboseLayerName);
  ui->outputMessages->addItem(tr("Verbose (console)"), GmicQt::VerboseConsole);
  ui->outputMessages->addItem(tr("Verbose (log file)"), GmicQt::VerboseLogFile);
  ui->outputMessages->addItem(tr("Very verbose (console)"), GmicQt::VeryVerboseConsole);
  ui->outputMessages->addItem(tr("Very verbose (log file)"), GmicQt::VeryVerboseLogFile);
  ui->outputMessages->addItem(tr("Debug (console)"), GmicQt::DebugConsole);
  ui->outputMessages->addItem(tr("Debug (log file)"), GmicQt::DebugLogFile);
  for (int index = 0; index < ui->outputMessages->count(); ++index) {
    if (ui->outputMessages->itemData(index) == _outputMessageMode) {
      ui->outputMessages->setCurrentIndex(index);
      break;
    }
  }

  ui->sbPreviewTimeout->setRange(1, 360);

  ui->rbLeftPreview->setChecked(_previewPosition == MainWindow::PreviewOnLeft);
  ui->rbRightPreview->setChecked(_previewPosition == MainWindow::PreviewOnRight);
  const bool savedDarkTheme = QSettings().value(DARK_THEME_KEY, false).toBool();
  ui->rbDarkTheme->setChecked(savedDarkTheme);
  ui->rbDefaultTheme->setChecked(!savedDarkTheme);
  ui->cbNativeColorDialogs->setChecked(_nativeColorDialogs);
  ui->cbNativeColorDialogs->setToolTip(tr("Check to use Native/OS color dialog, uncheck to use Qt's"));
  ui->cbShowLogos->setChecked(_logosAreVisible);
  ui->sbPreviewTimeout->setValue(_previewTimeout);

  ui->cbPreviewZoom->setChecked(_previewZoomAlwaysEnabled);

  connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(onOk()));
  connect(ui->rbLeftPreview, SIGNAL(toggled(bool)), this, SLOT(onRadioLeftPreviewToggled(bool)));
  connect(ui->pbUpdate, SIGNAL(clicked(bool)), this, SLOT(onUpdateClicked()));

  connect(ui->cbUpdatePeriodicity, SIGNAL(currentIndexChanged(int)), this, SLOT(onUpdatePeriodicityChanged(int)));

  connect(ui->labelPreviewLeft, SIGNAL(clicked()), ui->rbLeftPreview, SLOT(click()));
  connect(ui->labelPreviewRight, SIGNAL(clicked()), ui->rbRightPreview, SLOT(click()));

  connect(ui->cbNativeColorDialogs, SIGNAL(toggled(bool)), this, SLOT(onColorDialogsToggled(bool)));

  connect(Updater::getInstance(), SIGNAL(updateIsDone(int)), this, SLOT(enableUpdateButton()));

  connect(ui->rbDarkTheme, SIGNAL(toggled(bool)), this, SLOT(onDarkThemeToggled(bool)));

  connect(ui->cbShowLogos, SIGNAL(toggled(bool)), this, SLOT(onLogosVisibleToggled(bool)));

  connect(ui->cbPreviewZoom, SIGNAL(toggled(bool)), this, SLOT(onPreviewZoomToggled(bool)));

  connect(ui->sbPreviewTimeout, SIGNAL(valueChanged(int)), this, SLOT(onPreviewTimeoutChange(int)));

  connect(ui->outputMessages, SIGNAL(currentIndexChanged(int)), this, SLOT(onOutputMessageModeChanged(int)));

  ui->languageSelector->selectLanguage(_languageCode);
  if (_darkThemeEnabled) {
    QPalette p = ui->cbNativeColorDialogs->palette();
    p.setColor(QPalette::Text, DialogSettings::CheckBoxTextColor);
    p.setColor(QPalette::Base, DialogSettings::CheckBoxBaseColor);
    ui->cbNativeColorDialogs->setPalette(p);
    ui->cbPreviewZoom->setPalette(p);
    ui->cbUpdatePeriodicity->setPalette(p);
    ui->rbDarkTheme->setPalette(p);
    ui->rbDefaultTheme->setPalette(p);
    ui->rbLeftPreview->setPalette(p);
    ui->rbRightPreview->setPalette(p);
    ui->cbShowLogos->setPalette(p);
  }
  ui->pbOk->setFocus();
}

DialogSettings::~DialogSettings()
{
  delete ui;
}

void DialogSettings::loadSettings()
{
  QSettings settings;
  if (settings.value("Config/PreviewPosition", "Left").toString() == "Left") {
    _previewPosition = MainWindow::PreviewOnLeft;
  } else {
    _previewPosition = MainWindow::PreviewOnRight;
  }
  _darkThemeEnabled = settings.value(DARK_THEME_KEY, false).toBool();
  _languageCode = settings.value("Config/LanguageCode", QString()).toString();
  _nativeColorDialogs = settings.value("Config/NativeColorDialogs", false).toBool();
  _updatePeriodicity = settings.value(INTERNET_UPDATE_PERIODICITY_KEY, INTERNET_DEFAULT_PERIODICITY).toInt();
  FolderParameterDefaultValue = settings.value("FolderParameterDefaultValue", QDir::homePath()).toString();
  FileParameterDefaultPath = settings.value("FileParameterDefaultPath", QDir::homePath()).toString();
  _logosAreVisible = settings.value("LogosAreVisible", true).toBool();
  _previewTimeout = settings.value("PreviewTimeout", 16).toInt();
  _previewZoomAlwaysEnabled = settings.value("AlwaysEnablePreviewZoom", false).toBool();
  _outputMessageMode = static_cast<GmicQt::OutputMessageMode>(settings.value("OutputMessageMode", GmicQt::DefaultOutputMessageMode).toInt());
  // Do not try to create pixmaps if not a GUI application (headlesss mode)
  if (typeid(*qApp) != typeid(QCoreApplication)) {
    AddIcon = LOAD_ICON("list-add");
    RemoveIcon = LOAD_ICON("list-remove");
  }
}

bool DialogSettings::previewZoomAlwaysEnabled()
{
  return _previewZoomAlwaysEnabled;
}

int DialogSettings::previewTimeout()
{
  return _previewTimeout;
}

GmicQt::OutputMessageMode DialogSettings::outputMessageMode()
{
  return _outputMessageMode;
}

void DialogSettings::saveSettings(QSettings & settings)
{
  settings.setValue("Config/PreviewPosition", (_previewPosition == MainWindow::PreviewOnLeft) ? "Left" : "Right");
  settings.setValue("Config/NativeColorDialogs", _nativeColorDialogs);
  settings.setValue(INTERNET_UPDATE_PERIODICITY_KEY, _updatePeriodicity);
  settings.setValue("FolderParameterDefaultValue", FolderParameterDefaultValue);
  settings.setValue("FileParameterDefaultPath", FileParameterDefaultPath);
  settings.setValue("LogosAreVisible", _logosAreVisible);
  settings.setValue("PreviewTimeout", _previewTimeout);
  settings.setValue("OutputMessageMode", _outputMessageMode);
  settings.setValue("AlwaysEnablePreviewZoom", _previewZoomAlwaysEnabled);
  // Remove obsolete keys (2.0.0 pre-release)
  settings.remove("Config/UseFaveInputMode");
  settings.remove("Config/UseFaveOutputMode");
  settings.remove("Config/UseFaveOutputMessages");
  settings.remove("Config/UseFavePreviewMode");
}

MainWindow::PreviewPosition DialogSettings::previewPosition()
{
  return _previewPosition;
}

bool DialogSettings::logosAreVisible()
{
  return _logosAreVisible;
}

void DialogSettings::onOk()
{
  done(QDialog::Accepted);
}

void DialogSettings::done(int r)
{
  QSettings settings;
  saveSettings(settings);
  settings.setValue(DARK_THEME_KEY, ui->rbDarkTheme->isChecked());
  settings.setValue("Config/LanguageCode", ui->languageSelector->selectedLanguageCode());
  QDialog::done(r);
}

void DialogSettings::onLogosVisibleToggled(bool on)
{
  _logosAreVisible = on;
}

void DialogSettings::onPreviewTimeoutChange(int value)
{
  _previewTimeout = value;
}

void DialogSettings::onOutputMessageModeChanged(int)
{
  _outputMessageMode = static_cast<GmicQt::OutputMessageMode>(ui->outputMessages->currentData().toInt());
  Logger::setMode(_outputMessageMode);
}

void DialogSettings::onPreviewZoomToggled(bool on)
{
  _previewZoomAlwaysEnabled = on;
}

void DialogSettings::enableUpdateButton()
{
  ui->pbUpdate->setEnabled(true);
}

void DialogSettings::onRadioLeftPreviewToggled(bool on)
{
  if (on) {
    _previewPosition = MainWindow::PreviewOnLeft;
  } else {
    _previewPosition = MainWindow::PreviewOnRight;
  }
}

void DialogSettings::onUpdateClicked()
{
  MainWindow * mainWindow = dynamic_cast<MainWindow *>(parent());
  if (mainWindow) {
    ui->pbUpdate->setEnabled(false);
    mainWindow->updateFiltersFromSources(0, true);
  }
}

void DialogSettings::onDarkThemeToggled(bool) {}

void DialogSettings::onUpdatePeriodicityChanged(int)
{
  _updatePeriodicity = ui->cbUpdatePeriodicity->currentData().toInt();
}

void DialogSettings::onColorDialogsToggled(bool on)
{
  _nativeColorDialogs = on;
}

bool DialogSettings::darkThemeEnabled()
{
  return _darkThemeEnabled;
}

QString DialogSettings::languageCode()
{
  return _languageCode;
}

bool DialogSettings::nativeColorDialogs()
{
  return _nativeColorDialogs;
}
