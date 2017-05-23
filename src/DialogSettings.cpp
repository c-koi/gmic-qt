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
#include "ui_dialogsettings.h"
#include <QSettings>
#include <QCloseEvent>
#include <limits>
#include "Common.h"
#include "Updater.h"

bool DialogSettings::_darkThemeEnabled;
bool DialogSettings::_nativeColorDialogs;
MainWindow::PreviewPosition DialogSettings::_previewPosition;
int DialogSettings::_updatePeriodicity;

const QColor DialogSettings::CheckBoxBaseColor(83,83,83);
const QColor DialogSettings::CheckBoxTextColor(255,255,255);
QColor DialogSettings::UnselectedFilterTextColor;

QString DialogSettings::FolderParameterDefaultValue;
QString DialogSettings::FileParameterDefaultPath;

DialogSettings::DialogSettings(QWidget * parent):
  QDialog(parent),
  ui(new Ui::DialogSettings)
{
  ui->setupUi(this);

  setWindowTitle(tr("Settings"));
  setWindowIcon(parent->windowIcon());
  adjustSize();

  ui->pbUpdate->setIcon(LOAD_ICON("view-refresh"));

  loadSettings();

  ui->cbUpdatePeriodicity->addItem(tr("Never"),QVariant( INTERNET_NEVER_UPDATE_PERIODICITY ));
  ui->cbUpdatePeriodicity->addItem(tr("Daily"),QVariant(24));
  ui->cbUpdatePeriodicity->addItem(tr("Weekly"),QVariant(7*24));
  ui->cbUpdatePeriodicity->addItem(tr("Every 2 weeks"),QVariant(14*24));
  ui->cbUpdatePeriodicity->addItem(tr("Monthly"),QVariant(30*24));
#ifdef _GMIC_QT_DEBUG_
  ui->cbUpdatePeriodicity->addItem(tr("At launch (debug)"),QVariant(0));
#endif
  for ( int i = 0 ; i < ui->cbUpdatePeriodicity->count(); ++i) {
    if ( _updatePeriodicity == ui->cbUpdatePeriodicity->itemData(i).toInt() ) {
      ui->cbUpdatePeriodicity->setCurrentIndex(i);
    }
  }

  ui->rbLeftPreview->setChecked(_previewPosition == MainWindow::PreviewOnLeft);
  ui->rbRightPreview->setChecked(_previewPosition == MainWindow::PreviewOnRight);
  ui->rbDarkTheme->setChecked(_darkThemeEnabled);
  ui->rbDefaultTheme->setChecked(!_darkThemeEnabled);
  ui->cbNativeColorDialogs->setChecked(_nativeColorDialogs);
  ui->cbNativeColorDialogs->setToolTip(tr("Check to use Native/OS color dialog, uncheck to use Qt's"));

  connect(ui->pbOk,SIGNAL(clicked()),
          this,SLOT(onOk()));
  connect(ui->rbLeftPreview,SIGNAL(toggled(bool)),
          this,SLOT(onRadioLeftPreviewToggled(bool)));
  connect(ui->pbUpdate,SIGNAL(clicked(bool)),
          this,SLOT(onUpdateClicked()));

  connect(ui->cbUpdatePeriodicity,SIGNAL(currentIndexChanged(int)),
          this,SLOT(onUpdatePeriodicityChanged(int)));

  connect(ui->labelPreviewLeft,SIGNAL(clicked()),
          ui->rbLeftPreview,SLOT(click()));
  connect(ui->labelPreviewRight,SIGNAL(clicked()),
          ui->rbRightPreview,SLOT(click()));

  connect(ui->cbNativeColorDialogs,SIGNAL(toggled(bool)),
          this,SLOT(onColorDialogsToggled(bool)));


  connect(Updater::getInstance(),SIGNAL(downloadsFinished(bool)),
          this,SLOT(enableUpdateButton()));

  connect(ui->rbDarkTheme,SIGNAL(toggled(bool)),
          this,SLOT(onDarkThemeToggled(bool)));

  if ( _darkThemeEnabled ) {
    QPalette p = ui->cbNativeColorDialogs->palette();
    p.setColor(QPalette::Text, DialogSettings::CheckBoxTextColor);
    p.setColor(QPalette::Base, DialogSettings::CheckBoxBaseColor);
    ui->cbNativeColorDialogs->setPalette(p);
    ui->cbUpdatePeriodicity->setPalette(p);
    ui->rbDarkTheme->setPalette(p);
    ui->rbDefaultTheme->setPalette(p);
    ui->rbLeftPreview->setPalette(p);
    ui->rbRightPreview->setPalette(p);
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
  if ( settings.value("Config/PreviewPosition","Left").toString() == "Left" ) {
    _previewPosition = MainWindow::PreviewOnLeft;
  } else {
    _previewPosition = MainWindow::PreviewOnRight;
  }
  _darkThemeEnabled = settings.value("Config/DarkTheme",false).toBool();
  _nativeColorDialogs = settings.value("Config/NativeColorDialogs",false).toBool();
  _updatePeriodicity = settings.value(INTERNET_UPDATE_PERIODICITY_KEY,INTERNET_NEVER_UPDATE_PERIODICITY).toInt();

  FolderParameterDefaultValue = settings.value("FolderParameterDefaultValue",QDir::homePath()).toString();
  FileParameterDefaultPath = settings.value("FileParameterDefaultPath",QDir::homePath()).toString();
}

void DialogSettings::saveSettings(QSettings & settings)
{
  settings.setValue("Config/PreviewPosition",(_previewPosition==MainWindow::PreviewOnLeft)?"Left":"Right");
  settings.setValue("Config/DarkTheme",_darkThemeEnabled);
  settings.setValue("Config/NativeColorDialogs",_nativeColorDialogs);
  settings.setValue(INTERNET_UPDATE_PERIODICITY_KEY,_updatePeriodicity);
  settings.setValue("FolderParameterDefaultValue",FolderParameterDefaultValue);
  settings.setValue("FileParameterDefaultPath",FileParameterDefaultPath);

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

void DialogSettings::onRadioLeftPreviewToggled(bool on)
{
  if ( on ) {
    _previewPosition = MainWindow::PreviewOnLeft;
  } else {
    _previewPosition = MainWindow::PreviewOnRight;
  }
}

void DialogSettings::onUpdateClicked()
{
  MainWindow * mainWindow = dynamic_cast<MainWindow*>(parent());
  if ( mainWindow ) {
    ui->pbUpdate->setEnabled(false);
    mainWindow->updateFiltersFromSources(0,true);
  }
}

void DialogSettings::onOk()
{
  done(QDialog::Accepted);
}

void DialogSettings::enableUpdateButton()
{
  ui->pbUpdate->setEnabled(true);
}

void DialogSettings::onDarkThemeToggled(bool on)
{
  _darkThemeEnabled = on;
}

void DialogSettings::onUpdatePeriodicityChanged(int )
{
  _updatePeriodicity = ui->cbUpdatePeriodicity->currentData().toInt();
}

void DialogSettings::onColorDialogsToggled(bool on)
{
  _nativeColorDialogs = on;
}

void DialogSettings::done(int r)
{
  QSettings settings;
  saveSettings(settings);
  QDialog::done(r);
}

bool DialogSettings::darkThemeEnabled()
{
  return _darkThemeEnabled;
}

bool DialogSettings::nativeColorDialogs()
{
  return _nativeColorDialogs;
}
