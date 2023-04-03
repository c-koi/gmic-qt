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
#include "MainWindow.h"
#include "Settings.h"
#include "Updater.h"
#include "ui_dialogsettings.h"

namespace GmicQt
{
DialogSettings::DialogSettings(QWidget * parent) : QDialog(parent), ui(new Ui::DialogSettings)
{
  ui->setupUi(this);

  setWindowTitle(tr("Settings"));
  setWindowIcon(parent->windowIcon());
  adjustSize();

  ui->pbUpdate->setIcon(IconLoader::load("view-refresh"));

  ui->cbUpdatePeriodicity->addItem(tr("Never"), QVariant(INTERNET_NEVER_UPDATE_PERIODICITY));
  ui->cbUpdatePeriodicity->addItem(tr("Daily"), QVariant(ONE_DAY_HOURS));
  ui->cbUpdatePeriodicity->addItem(tr("Weekly"), QVariant(ONE_WEEK_HOURS));
  ui->cbUpdatePeriodicity->addItem(tr("Every 2 weeks"), QVariant(TWO_WEEKS_HOURS));
  ui->cbUpdatePeriodicity->addItem(tr("Monthly"), QVariant(ONE_MONTH_HOURS));
#ifdef _GMIC_QT_DEBUG_
  ui->cbUpdatePeriodicity->addItem(tr("At launch (debug)"), QVariant(0));
#endif
  for (int i = 0; i < ui->cbUpdatePeriodicity->count(); ++i) {
    if (Settings::updatePeriodicity() == ui->cbUpdatePeriodicity->itemData(i).toInt()) {
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
    if (ui->outputMessages->itemData(index) == (int)Settings::outputMessageMode()) {
      ui->outputMessages->setCurrentIndex(index);
      break;
    }
  }

  ui->sbPreviewTimeout->setRange(0, 999);

  ui->rbLeftPreview->setChecked(Settings::previewPosition() == MainWindow::PreviewPosition::Left);
  ui->rbRightPreview->setChecked(Settings::previewPosition() == MainWindow::PreviewPosition::Right);
  const bool savedDarkTheme = QSettings().value(DARK_THEME_KEY, GmicQtHost::DarkThemeIsDefault).toBool();
  ui->rbDarkTheme->setChecked(savedDarkTheme);
  ui->rbDefaultTheme->setChecked(!savedDarkTheme);
  ui->cbNativeColorDialogs->setChecked(Settings::nativeColorDialogs());
  ui->cbNativeColorDialogs->setToolTip(tr("Check to use Native/OS color dialog, uncheck to use Qt's"));
  ui->cbNativeFileDialogs->setChecked(Settings::nativeFileDialogs());
  ui->cbNativeFileDialogs->setToolTip(tr("Check to use Native/OS file dialog, uncheck to use Qt's"));
  ui->cbShowLogos->setChecked(Settings::visibleLogos());
  ui->sbPreviewTimeout->setValue(Settings::previewTimeout());
  ui->cbPreviewZoom->setChecked(Settings::previewZoomAlwaysEnabled());
  ui->cbNotifyFailedUpdate->setChecked(Settings::notifyFailedStartupUpdate());

  connect(ui->pbOk, &QPushButton::clicked, this, &DialogSettings::onOk);
  connect(ui->rbLeftPreview, &QRadioButton::toggled, this, &DialogSettings::onRadioLeftPreviewToggled);
  connect(ui->pbUpdate, &QPushButton::clicked, this, &DialogSettings::onUpdateClicked);
  connect(ui->cbUpdatePeriodicity, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DialogSettings::onUpdatePeriodicityChanged);
  connect(ui->labelPreviewLeft, &ClickableLabel::clicked, ui->rbLeftPreview, &QRadioButton::click);
  connect(ui->labelPreviewRight, &ClickableLabel::clicked, ui->rbRightPreview, &QRadioButton::click);
  connect(ui->cbNativeColorDialogs, &QCheckBox::toggled, this, &DialogSettings::onColorDialogsToggled);
  connect(ui->cbNativeFileDialogs, &QCheckBox::toggled, this, &DialogSettings::onFileDialogsToggled);
  connect(Updater::getInstance(), &Updater::updateIsDone, this, &DialogSettings::enableUpdateButton);
  connect(ui->rbDarkTheme, &QRadioButton::toggled, this, &DialogSettings::onDarkThemeToggled);
  connect(ui->cbShowLogos, &QCheckBox::toggled, this, &DialogSettings::onVisibleLogosToggled);
  connect(ui->cbPreviewZoom, &QCheckBox::toggled, this, &DialogSettings::onPreviewZoomToggled);
  connect(ui->sbPreviewTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this, &DialogSettings::onPreviewTimeoutChange);
  connect(ui->outputMessages, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DialogSettings::onOutputMessageModeChanged);
  connect(ui->cbNotifyFailedUpdate, &QCheckBox::toggled, this, &DialogSettings::onNotifyStartupUpdateFailedToggle);

#if QT_VERSION_GTE(6, 0, 0)
  ui->cbHighDPI->hide();
  ui->labelHighDPI->hide();
#else
  ui->cbHighDPI->setChecked(Settings::highDPIEnabled());
  connect(ui->cbHighDPI, &QCheckBox::toggled, this, &DialogSettings::onHighDPIToggled);
#endif

  ui->languageSelector->selectLanguage(Settings::languageCode());
  ui->languageSelector->enableFilterTranslation(Settings::filterTranslationEnabled());

  if (Settings::darkThemeEnabled()) {
    QPalette p = ui->cbNativeColorDialogs->palette();
    p.setColor(QPalette::Text, Settings::CheckBoxTextColor);
    p.setColor(QPalette::Base, Settings::CheckBoxBaseColor);
    ui->cbNativeColorDialogs->setPalette(p);
    ui->cbNativeFileDialogs->setPalette(p);
    ui->cbPreviewZoom->setPalette(p);
    ui->cbUpdatePeriodicity->setPalette(p);
    ui->rbDarkTheme->setPalette(p);
    ui->rbDefaultTheme->setPalette(p);
    ui->rbLeftPreview->setPalette(p);
    ui->rbRightPreview->setPalette(p);
    ui->cbShowLogos->setPalette(p);
    ui->cbNotifyFailedUpdate->setPalette(p);
    ui->cbHighDPI->setPalette(p);
  }
  ui->pbOk->setFocus();
  ui->tabWidget->setCurrentIndex(0);
}

DialogSettings::~DialogSettings()
{
  delete ui;
}

void DialogSettings::sourcesStatus(bool & modified, bool & internetUpdateRequired)
{
  modified = ui->sources->sourcesModified(internetUpdateRequired);
}

void DialogSettings::onOk()
{
  done(QDialog::Accepted);
}

void DialogSettings::done(int r)
{
  QSettings settings;
  ui->sources->saveSettings();
  Settings::save(settings);
  QDialog::done(r);
}

void DialogSettings::onVisibleLogosToggled(bool on)
{
  Settings::setVisibleLogos(on);
}

void DialogSettings::onPreviewTimeoutChange(int value)
{
  Settings::setPreviewTimeout(value);
}

void DialogSettings::onOutputMessageModeChanged(int)
{
  const OutputMessageMode mode = static_cast<OutputMessageMode>(ui->outputMessages->currentData().toInt());
  Settings::setOutputMessageMode(mode);
  Logger::setMode(mode);
}

void DialogSettings::onPreviewZoomToggled(bool on)
{
  Settings::setPreviewZoomAlwaysEnabled(on);
}

void DialogSettings::onNotifyStartupUpdateFailedToggle(bool on)
{
  Settings::setNotifyFailedStartupUpdate(on);
}

void DialogSettings::onHighDPIToggled(bool on)
{
  Settings::setHighDPIEnabled(on);
}

void DialogSettings::enableUpdateButton()
{
  ui->pbUpdate->setEnabled(true);
}

void DialogSettings::onRadioLeftPreviewToggled(bool on)
{
  if (on) {
    Settings::setPreviewPosition(MainWindow::PreviewPosition::Left);
  } else {
    Settings::setPreviewPosition(MainWindow::PreviewPosition::Right);
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

void DialogSettings::onDarkThemeToggled(bool on)
{
  QSettings().setValue(DARK_THEME_KEY, on);
}

void DialogSettings::onUpdatePeriodicityChanged(int)
{
  Settings::setUpdatePeriodicity(ui->cbUpdatePeriodicity->currentData().toInt());
}

void DialogSettings::onColorDialogsToggled(bool on)
{
  Settings::setNativeColorDialogs(on);
}

void DialogSettings::onFileDialogsToggled(bool on)
{
  Settings::setNativeFileDialogs(on);
}

} // namespace GmicQt
