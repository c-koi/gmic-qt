/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file InOutPanel.cpp
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
#include "Widgets/InOutPanel.h"
#include <QDebug>
#include <QPalette>
#include <QSettings>
#include <cstdio>
#include "DialogSettings.h"
#include "IconLoader.h"
#include "ui_inoutpanel.h"

/*
 * InOutPanel methods
 */

InOutPanel::InOutPanel(QWidget * parent) : QWidget(parent), ui(new Ui::InOutPanel)
{
  ui->setupUi(this);

  ui->topLabel->setStyleSheet("QLabel { font-weight: bold }");
  ui->tbReset->setIcon(LOAD_ICON("view-refresh"));

  ui->inputLayers->setToolTip(tr("Input layers"));
  ui->inputLayers->addItem(tr("None"), GmicQt::NoInput);
  ui->inputLayers->addItem(tr("Active (default)"), GmicQt::Active);
  ui->inputLayers->addItem(tr("All"), GmicQt::All);
  ui->inputLayers->addItem(tr("Active and below"), GmicQt::ActiveAndBelow);
  ui->inputLayers->addItem(tr("Active and above"), GmicQt::ActiveAndAbove);
  ui->inputLayers->addItem(tr("All visible"), GmicQt::AllVisibles);
  ui->inputLayers->addItem(tr("All invisible"), GmicQt::AllInvisibles);
  ui->inputLayers->addItem(tr("All visible (decr.)"), GmicQt::AllVisiblesDesc);
  ui->inputLayers->addItem(tr("All invisible (decr.)"), GmicQt::AllInvisiblesDesc);
  ui->inputLayers->addItem(tr("All (decr.)"), GmicQt::AllDesc);

  ui->outputMode->setToolTip(tr("Output mode"));
  ui->outputMode->addItem(tr("In place (default)"), GmicQt::InPlace);
  ui->outputMode->addItem(tr("New layer(s)"), GmicQt::NewLayers);
  ui->outputMode->addItem(tr("New active layer(s)"), GmicQt::NewActiveLayers);
  ui->outputMode->addItem(tr("New image"), GmicQt::NewImage);

  ui->previewMode->setToolTip(tr("Preview mode"));
  ui->previewMode->addItem(tr("1st output (default)"), GmicQt::FirstOutput);
  ui->previewMode->addItem(tr("2nd output"), GmicQt::SecondOutput);
  ui->previewMode->addItem(tr("3rd output"), GmicQt::ThirdOutput);
  ui->previewMode->addItem(tr("4th output"), GmicQt::FourthOutput);
  ui->previewMode->addItem(tr("1st -> 2nd output"), GmicQt::First2SecondOutput);
  ui->previewMode->addItem(tr("1st -> 3rd output"), GmicQt::First2ThirdOutput);
  ui->previewMode->addItem(tr("1st -> 4th output"), GmicQt::First2FourthOutput);
  ui->previewMode->addItem(tr("All outputs"), GmicQt::AllOutputs);

  connect(ui->inputLayers, SIGNAL(currentIndexChanged(int)), this, SLOT(onInputModeSelected(int)));
  connect(ui->outputMode, SIGNAL(currentIndexChanged(int)), this, SLOT(onOutputModeSelected(int)));
  connect(ui->previewMode, SIGNAL(currentIndexChanged(int)), this, SLOT(onPreviewModeSelected(int)));
  connect(ui->tbReset, SIGNAL(clicked(bool)), this, SLOT(onResetButtonClicked()));

  _notifyValueChange = true;
}

InOutPanel::~InOutPanel()
{
  delete ui;
}

GmicQt::InputMode InOutPanel::inputMode() const
{
  int mode = ui->inputLayers->currentData().toInt();
  return static_cast<GmicQt::InputMode>(mode);
}

GmicQt::OutputMode InOutPanel::outputMode() const
{
  int mode = ui->outputMode->currentData().toInt();
  return static_cast<GmicQt::OutputMode>(mode);
}

GmicQt::PreviewMode InOutPanel::previewMode() const
{
  int mode = ui->previewMode->currentData().toInt();
  return static_cast<GmicQt::PreviewMode>(mode);
}

void InOutPanel::setInputMode(GmicQt::InputMode mode)
{
  int index = ui->inputLayers->findData(mode);
  ui->inputLayers->setCurrentIndex((index == -1) ? ui->inputLayers->findData(GmicQt::DefaultInputMode) : index);
}

void InOutPanel::setOutputMode(GmicQt::OutputMode mode)
{
  int index = ui->outputMode->findData(mode);
  ui->outputMode->setCurrentIndex((index == -1) ? ui->outputMode->findData(GmicQt::DefaultOutputMode) : index);
}

void InOutPanel::setPreviewMode(GmicQt::PreviewMode mode)
{
  int index = ui->previewMode->findData(mode);
  ui->previewMode->setCurrentIndex((index == -1) ? ui->previewMode->findData(GmicQt::DefaultPreviewMode) : index);
}

void InOutPanel::reset()
{
  ui->inputLayers->setCurrentIndex(ui->inputLayers->findData(GmicQt::DefaultInputMode));
  ui->outputMode->setCurrentIndex(ui->outputMode->findData(GmicQt::DefaultOutputMode));
  ui->previewMode->setCurrentIndex(ui->previewMode->findData(GmicQt::DefaultPreviewMode));
}

void InOutPanel::onInputModeSelected(int)
{
  if (_notifyValueChange) {
    emit inputModeChanged(inputMode());
  }
}

void InOutPanel::onOutputModeSelected(int) {}

void InOutPanel::onPreviewModeSelected(int)
{
  if (_notifyValueChange) {
    emit previewModeChanged(previewMode());
  }
}

void InOutPanel::onResetButtonClicked()
{
  setState(GmicQt::InputOutputState::Default, true);
}

void InOutPanel::setDarkTheme()
{
  ui->tbReset->setIcon(LOAD_ICON("view-refresh"));
}

void InOutPanel::disableNotifications()
{
  _notifyValueChange = false;
}

void InOutPanel::enableNotifications()
{
  _notifyValueChange = true;
}

/*
 * InOutPanel::state methods
 */

GmicQt::InputOutputState InOutPanel::state() const
{
  return {inputMode(), outputMode(), previewMode()};
}

void InOutPanel::setState(const GmicQt::InputOutputState & state, bool notify)
{
  bool savedNotificationStatus = _notifyValueChange;
  if (notify) {
    enableNotifications();
  } else {
    disableNotifications();
  }

  setInputMode(state.inputMode);
  setOutputMode(state.outputMode);
  setPreviewMode(state.previewMode);

  if (savedNotificationStatus) {
    enableNotifications();
  } else {
    disableNotifications();
  }
}

void InOutPanel::setEnabled(bool on)
{
  ui->inputLayers->setEnabled(on);
  ui->outputMode->setEnabled(on);
  ui->previewMode->setEnabled(on);
}

void InOutPanel::disable()
{
  setEnabled(false);
}

void InOutPanel::enable()
{
  setEnabled(true);
}
