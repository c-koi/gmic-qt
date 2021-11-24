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
#include "Host/GmicQtHost.h"
#include "IconLoader.h"
#include "Logger.h"
#include "Updater.h"
#include "ui_dialogsettings.h"

namespace
{
GmicQt::OutputMessageMode filterDeprecatedOutputMessageMode(const GmicQt::OutputMessageMode & mode)
{
  if (mode == GmicQt::OutputMessageMode::VerboseLayerName_DEPRECATED) {
    return GmicQt::DefaultOutputMessageMode;
  }
  return mode;
}
} // namespace

namespace GmicQt
{

bool DialogSettings::_darkThemeEnabled;
QString DialogSettings::_languageCode;
bool DialogSettings::_nativeColorDialogs;
bool DialogSettings::_logosAreVisible;
MainWindow::PreviewPosition DialogSettings::_previewPosition;
int DialogSettings::_updatePeriodicity;
OutputMessageMode DialogSettings::_outputMessageMode;
bool DialogSettings::_previewZoomAlwaysEnabled = false;
bool DialogSettings::_notifyFailedStartupUpdate = true;

const QColor DialogSettings::CheckBoxBaseColor(83, 83, 83);
const QColor DialogSettings::CheckBoxTextColor(255, 255, 255);
QColor DialogSettings::UnselectedFilterTextColor;

QString DialogSettings::FolderParameterDefaultValue;
QString DialogSettings::FileParameterDefaultPath;
int DialogSettings::_previewTimeout = 16;

QIcon DialogSettings::AddIcon;
QIcon DialogSettings::RemoveIcon;

QString DialogSettings::GroupSeparator;
QString DialogSettings::DecimalPoint('.');
QString DialogSettings::NegativeSign('-');
bool DialogSettings::_filterTranslationEnabled = false;
// TODO : Make DialogSetting a view of a Settings class

DialogSettings::DialogSettings(QWidget * parent) : QDialog(parent), ui(new Ui::DialogSettings)
{
  ui->setupUi(this);

  setWindowTitle(tr("Settings"));
  setWindowIcon(parent->windowIcon());
  adjustSize();

  ui->pbUpdate->setIcon(LOAD_ICON("view-refresh"));

  ui->cbUpdatePeriodicity->addItem(tr("Never"), QVariant(INTERNET_NEVER_UPDATE_PERIODICITY));
  ui->cbUpdatePeriodicity->addItem(tr("Daily"), QVariant(ONE_DAY_HOURS));
  ui->cbUpdatePeriodicity->addItem(tr("Weekly"), QVariant(ONE_WEEK_HOURS));
  ui->cbUpdatePeriodicity->addItem(tr("Every 2 weeks"), QVariant(TWO_WEEKS_HOURS));
  ui->cbUpdatePeriodicity->addItem(tr("Monthly"), QVariant(ONE_MONTH_HOURS));
#ifdef _GMIC_QT_DEBUG_
  ui->cbUpdatePeriodicity->addItem(tr("At launch (debug)"), QVariant(0));
#endif
  for (int i = 0; i < ui->cbUpdatePeriodicity->count(); ++i) {
    if (_updatePeriodicity == ui->cbUpdatePeriodicity->itemData(i).toInt()) {
      ui->cbUpdatePeriodicity->setCurrentIndex(i);
    }
  }

  ui->outputMessages->setToolTip(tr("Output messages"));
  ui->outputMessages->addItem(tr("Quiet (default)"), (int)OutputMessageMode::Quiet);
  ui->outputMessages->addItem(tr("Verbose (console)"), (int)OutputMessageMode::VerboseConsole);
  ui->outputMessages->addItem(tr("Verbose (log file)"), (int)OutputMessageMode::VerboseLogFile);
  ui->outputMessages->addItem(tr("Very verbose (console)"), (int)OutputMessageMode::VeryVerboseConsole);
  ui->outputMessages->addItem(tr("Very verbose (log file)"), (int)OutputMessageMode::VeryVerboseLogFile);
  ui->outputMessages->addItem(tr("Debug (console)"), (int)OutputMessageMode::DebugConsole);
  ui->outputMessages->addItem(tr("Debug (log file)"), (int)OutputMessageMode::DebugLogFile);
  for (int index = 0; index < ui->outputMessages->count(); ++index) {
    if (ui->outputMessages->itemData(index) == (int)_outputMessageMode) {
      ui->outputMessages->setCurrentIndex(index);
      break;
    }
  }

  ui->sbPreviewTimeout->setRange(0, 999);

  ui->rbLeftPreview->setChecked(_previewPosition == MainWindow::PreviewPosition::Left);
  ui->rbRightPreview->setChecked(_previewPosition == MainWindow::PreviewPosition::Right);
  const bool savedDarkTheme = QSettings().value(DARK_THEME_KEY, GmicQtHost::DarkThemeIsDefault).toBool();
  ui->rbDarkTheme->setChecked(savedDarkTheme);
  ui->rbDefaultTheme->setChecked(!savedDarkTheme);
  ui->cbNativeColorDialogs->setChecked(_nativeColorDialogs);
  ui->cbNativeColorDialogs->setToolTip(tr("Check to use Native/OS color dialog, uncheck to use Qt's"));
  ui->cbShowLogos->setChecked(_logosAreVisible);
  ui->sbPreviewTimeout->setValue(_previewTimeout);
  ui->cbPreviewZoom->setChecked(_previewZoomAlwaysEnabled);
  ui->cbNotifyFailedUpdate->setChecked(_notifyFailedStartupUpdate);

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

  connect(ui->cbNotifyFailedUpdate, SIGNAL(toggled(bool)), this, SLOT(onNotifyStartupUpdateFailedToggle(bool)));

  ui->languageSelector->selectLanguage(_languageCode);
  ui->languageSelector->enableFilterTranslation(_filterTranslationEnabled);

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
    ui->cbNotifyFailedUpdate->setPalette(p);
  }
  ui->pbOk->setFocus();
  ui->tabWidget->setCurrentIndex(0);
}

DialogSettings::~DialogSettings()
{
  delete ui;
}

void DialogSettings::loadSettings(UserInterfaceMode userInterfaceMode)
{
  QSettings settings;
  if (settings.value("Config/PreviewPosition", "Left").toString() == "Left") {
    _previewPosition = MainWindow::PreviewPosition::Left;
  } else {
    _previewPosition = MainWindow::PreviewPosition::Right;
  }
  _darkThemeEnabled = settings.value(DARK_THEME_KEY, GmicQtHost::DarkThemeIsDefault).toBool();
  _languageCode = settings.value("Config/LanguageCode", QString()).toString();
  _filterTranslationEnabled = settings.value(ENABLE_FILTER_TRANSLATION, false).toBool();
  _nativeColorDialogs = settings.value("Config/NativeColorDialogs", false).toBool();
  _updatePeriodicity = settings.value(INTERNET_UPDATE_PERIODICITY_KEY, INTERNET_DEFAULT_PERIODICITY).toInt();
  FolderParameterDefaultValue = settings.value("FolderParameterDefaultValue", QDir::homePath()).toString();
  FileParameterDefaultPath = settings.value("FileParameterDefaultPath", QDir::homePath()).toString();
  _logosAreVisible = settings.value("LogosAreVisible", true).toBool();
  _previewTimeout = settings.value("PreviewTimeout", 16).toInt();
  _previewZoomAlwaysEnabled = settings.value("AlwaysEnablePreviewZoom", false).toBool();
  _outputMessageMode = filterDeprecatedOutputMessageMode((GmicQt::OutputMessageMode)settings.value("OutputMessageMode", static_cast<int>(GmicQt::DefaultOutputMessageMode)).toInt());
  _notifyFailedStartupUpdate = settings.value("Config/NotifyIfStartupUpdateFails", true).toBool();
  if (userInterfaceMode != UserInterfaceMode::Silent) {
    AddIcon = LOAD_ICON("list-add");
    RemoveIcon = LOAD_ICON("list-remove");
  }
  QLocale locale;
  GroupSeparator = locale.groupSeparator();
  DecimalPoint = locale.decimalPoint();
  NegativeSign = locale.negativeSign();
}

bool DialogSettings::previewZoomAlwaysEnabled()
{
  return _previewZoomAlwaysEnabled;
}

bool DialogSettings::notifyFailedStartupUpdate()
{
  return _notifyFailedStartupUpdate;
}

int DialogSettings::previewTimeout()
{
  return _previewTimeout;
}

OutputMessageMode DialogSettings::outputMessageMode()
{
  return _outputMessageMode;
}

bool DialogSettings::filterTranslationEnabled()
{
  return _filterTranslationEnabled;
}

void DialogSettings::saveSettings(QSettings & settings)
{
  removeObsoleteKeys(settings);
  settings.setValue("Config/PreviewPosition", (_previewPosition == MainWindow::PreviewPosition::Left) ? "Left" : "Right");
  settings.setValue("Config/NativeColorDialogs", _nativeColorDialogs);
  settings.setValue(INTERNET_UPDATE_PERIODICITY_KEY, _updatePeriodicity);
  settings.setValue("FolderParameterDefaultValue", FolderParameterDefaultValue);
  settings.setValue("FileParameterDefaultPath", FileParameterDefaultPath);
  settings.setValue("LogosAreVisible", _logosAreVisible);
  settings.setValue("PreviewTimeout", _previewTimeout);
  settings.setValue("OutputMessageMode", (int)_outputMessageMode);
  settings.setValue("AlwaysEnablePreviewZoom", _previewZoomAlwaysEnabled);
  // Remove obsolete keys (2.0.0 pre-release)
  settings.remove("Config/UseFaveInputMode");
  settings.remove("Config/UseFaveOutputMode");
  settings.remove("Config/UseFaveOutputMessages");
  settings.remove("Config/UseFavePreviewMode");
  settings.setValue("Config/NotifyIfStartupUpdateFails", _notifyFailedStartupUpdate);
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
  settings.setValue(ENABLE_FILTER_TRANSLATION, ui->languageSelector->translateFiltersEnabled());
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
  _outputMessageMode = static_cast<OutputMessageMode>(ui->outputMessages->currentData().toInt());
  Logger::setMode(_outputMessageMode);
}

void DialogSettings::onPreviewZoomToggled(bool on)
{
  _previewZoomAlwaysEnabled = on;
}

void DialogSettings::onNotifyStartupUpdateFailedToggle(bool on)
{
  _notifyFailedStartupUpdate = on;
}

void DialogSettings::removeObsoleteKeys(QSettings & settings)
{
  settings.remove(QString("LastExecution/host_%1/PreviewMode").arg(GmicQtHost::ApplicationShortname));
  settings.remove(QString("LastExecution/host_%1/GmicEnvironment").arg(GmicQtHost::ApplicationShortname));
  settings.remove(QString("LastExecution/host_%1/QuotedParameters").arg(GmicQtHost::ApplicationShortname));
  settings.remove(QString("LastExecution/host_%1/GmicStatus").arg(GmicQtHost::ApplicationShortname));
}

void DialogSettings::enableUpdateButton()
{
  ui->pbUpdate->setEnabled(true);
}

void DialogSettings::onRadioLeftPreviewToggled(bool on)
{
  if (on) {
    _previewPosition = MainWindow::PreviewPosition::Left;
  } else {
    _previewPosition = MainWindow::PreviewPosition::Right;
  }
}

void DialogSettings::onUpdateClicked()
{
  auto mainWindow = dynamic_cast<MainWindow *>(parent());
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

} // namespace GmicQt
