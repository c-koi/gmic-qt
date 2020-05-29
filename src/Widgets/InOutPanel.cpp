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

QList<GmicQt::InputMode> InOutPanel::_enabledInputModes = {
    GmicQt::NoInput,        //
    GmicQt::Active,         //
    GmicQt::All,            //
    GmicQt::ActiveAndBelow, //
    GmicQt::ActiveAndAbove, //
    GmicQt::AllVisible,     //
    GmicQt::AllInvisible,
};

QList<GmicQt::OutputMode> InOutPanel::_enabledOutputModes = {
    GmicQt::InPlace,
    GmicQt::NewLayers,
    GmicQt::NewActiveLayers,
    GmicQt::NewImage,
};

QList<GmicQt::PreviewMode> InOutPanel::_enabledPreviewModes = { //
    GmicQt::FirstOutput,                                        //
    GmicQt::SecondOutput,                                       //
    GmicQt::ThirdOutput,                                        //
    GmicQt::FourthOutput,                                       //
    GmicQt::First2SecondOutput,                                 //
    GmicQt::First2ThirdOutput,                                  //
    GmicQt::First2FourthOutput,                                 //
    GmicQt::AllOutputs};

/*
 * InOutPanel methods
 */

InOutPanel::InOutPanel(QWidget * parent) : QWidget(parent), ui(new Ui::InOutPanel)
{
  ui->setupUi(this);

  ui->topLabel->setStyleSheet("QLabel { font-weight: bold }");
  ui->tbReset->setIcon(LOAD_ICON("view-refresh"));

  ui->inputLayers->setToolTip(tr("Input layers"));

#define ADD_INPUT_IF_ENABLED(MODE, TEXT)                                                                                                                                                               \
  if (_enabledInputModes.contains(MODE))                                                                                                                                                               \
  ui->inputLayers->addItem(tr(TEXT), MODE)

  ADD_INPUT_IF_ENABLED(GmicQt::NoInput, "None");
  ADD_INPUT_IF_ENABLED(GmicQt::Active, "Active (default)");
  ADD_INPUT_IF_ENABLED(GmicQt::All, "All");
  ADD_INPUT_IF_ENABLED(GmicQt::ActiveAndBelow, "Active and below");
  ADD_INPUT_IF_ENABLED(GmicQt::ActiveAndAbove, "Active and above");
  ADD_INPUT_IF_ENABLED(GmicQt::AllVisible, "All visible");
  ADD_INPUT_IF_ENABLED(GmicQt::AllInvisible, "All invisible");
  // "decr." input mode have been removed (since 2.8.2)
  //  ui->inputLayers->addItem(tr("All visible (decr.)"), GmicQt::AllVisiblesDesc);
  //  ui->inputLayers->addItem(tr("All invisible (decr.)"), GmicQt::AllInvisiblesDesc);
  //  ui->inputLayers->addItem(tr("All (decr.)"), GmicQt::AllDesc);

  if (ui->inputLayers->count() == 1) {
    ui->labelInputLayers->hide();
    ui->inputLayers->hide();
  }

  ui->outputMode->setToolTip(tr("Output mode"));

#define ADD_OUTPUT_IF_ENABLED(MODE, TEXT)                                                                                                                                                              \
  if (_enabledOutputModes.contains(MODE))                                                                                                                                                              \
  ui->outputMode->addItem(tr(TEXT), MODE)

  ADD_OUTPUT_IF_ENABLED(GmicQt::InPlace, "In place (default)");
  ADD_OUTPUT_IF_ENABLED(GmicQt::NewLayers, "New layer(s)");
  ADD_OUTPUT_IF_ENABLED(GmicQt::NewActiveLayers, "New active layer(s)");
  ADD_OUTPUT_IF_ENABLED(GmicQt::NewImage, "New image");

  if (ui->outputMode->count() == 1) {
    ui->labelOutputMode->hide();
    ui->outputMode->hide();
  }

  ui->previewMode->setToolTip(tr("Preview mode"));

#define ADD_PREVIEW_IF_ENABLED(MODE, TEXT)                                                                                                                                                             \
  if (_enabledPreviewModes.contains(MODE))                                                                                                                                                             \
  ui->previewMode->addItem(tr(TEXT), MODE)

  ADD_PREVIEW_IF_ENABLED(GmicQt::FirstOutput, "1st output (default)");
  ADD_PREVIEW_IF_ENABLED(GmicQt::SecondOutput, "2nd output");
  ADD_PREVIEW_IF_ENABLED(GmicQt::ThirdOutput, "3rd output");
  ADD_PREVIEW_IF_ENABLED(GmicQt::FourthOutput, "4th output");
  ADD_PREVIEW_IF_ENABLED(GmicQt::First2SecondOutput, "1st -> 2nd output");
  ADD_PREVIEW_IF_ENABLED(GmicQt::First2ThirdOutput, "1st -> 3rd output");
  ADD_PREVIEW_IF_ENABLED(GmicQt::First2FourthOutput, "1st -> 4th output");
  ADD_PREVIEW_IF_ENABLED(GmicQt::AllOutputs, "All outputs");

  if (ui->previewMode->count() == 1) {
    ui->labelPreviewMode->hide();
    ui->previewMode->hide();
  }

  setDefaultInputMode();
  setDefaultOutputMode();
  setDefaultPreviewMode();
  setTopLabel();
  updateLayoutIfUniqueRow();

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

void InOutPanel::setDefaultInputMode()
{
  if (_enabledInputModes.contains(GmicQt::DefaultInputMode)) {
    return;
  }
  for (int m = GmicQt::Active; m <= GmicQt::AllInvisible; ++m) {
    auto mode = static_cast<GmicQt::InputMode>(m);
    if (_enabledInputModes.contains(mode)) {
      GmicQt::DefaultInputMode = mode;
      return;
    }
  }
  Q_ASSERT_X(_enabledInputModes.contains(GmicQt::NoInput), __FUNCTION__, "No input mode left by host settings. Default mode cannot be determined.");
  GmicQt::DefaultInputMode = GmicQt::NoInput;
}

void InOutPanel::setDefaultOutputMode()
{
  if (_enabledOutputModes.contains(GmicQt::DefaultOutputMode)) {
    return;
  }
  Q_ASSERT_X(!_enabledOutputModes.isEmpty(), __FUNCTION__, "No output mode left by host settings. Default mode cannot be determined.");
  for (int m = GmicQt::InPlace; m <= GmicQt::NewImage; ++m) {
    auto mode = static_cast<GmicQt::OutputMode>(m);
    if (_enabledOutputModes.contains(mode)) {
      GmicQt::DefaultOutputMode = mode;
      return;
    }
  }
}

void InOutPanel::setDefaultPreviewMode()
{
  if (_enabledPreviewModes.contains(GmicQt::DefaultPreviewMode)) {
    return;
  }
  Q_ASSERT_X(!_enabledPreviewModes.isEmpty(), __FUNCTION__, "No preview mode left by host settings. Default mode cannot be determined.");
  for (int m = GmicQt::FirstOutput; m <= GmicQt::AllOutputs; ++m) {
    auto mode = static_cast<GmicQt::PreviewMode>(m);
    if (_enabledPreviewModes.contains(mode)) {
      GmicQt::DefaultPreviewMode = mode;
      return;
    }
  }
}

void InOutPanel::setTopLabel()
{
  const bool input = ui->inputLayers->count() > 1;
  const bool output = ui->outputMode->count() > 1;
  const bool preview = ui->previewMode->count() > 1;
  if (input && output) {
    ui->topLabel->setText(tr("Input / Output"));
  } else if (input) {
    ui->topLabel->setText(preview ? tr("Input / Preview") : tr("Input"));
  } else if (output) {
    ui->topLabel->setText(preview ? tr("Output / Preview") : tr("Output"));
  } else if (preview) {
    ui->topLabel->setText("Preview");
  }
}

void InOutPanel::updateLayoutIfUniqueRow()
{
  const bool input = ui->inputLayers->count() > 1;
  const bool output = ui->outputMode->count() > 1;
  const bool preview = ui->previewMode->count() > 1;
  if ((input + output + preview) > 1) {
    return;
  }
  if (input) {
    ui->topLabel->setText(ui->labelInputLayers->text());
    ui->horizontalLayout->insertWidget(1, ui->inputLayers);
  } else if (output) {
    ui->topLabel->setText(ui->labelOutputMode->text());
    ui->horizontalLayout->insertWidget(1, ui->outputMode);
  } else if (preview) {
    ui->topLabel->setText(ui->labelPreviewMode->text());
    ui->horizontalLayout->insertWidget(1, ui->previewMode);
  }
  ui->topLabel->setStyleSheet("QLabel { font-weight: normal }");
  ui->scrollArea->hide();
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

void InOutPanel::disableInputMode(GmicQt::InputMode mode)
{
  _enabledInputModes.removeOne(mode);
}

void InOutPanel::disableOutputMode(GmicQt::OutputMode mode)
{
  _enabledOutputModes.removeOne(mode);
}

void InOutPanel::disablePreviewMode(GmicQt::PreviewMode mode)
{
  _enabledPreviewModes.removeOne(mode);
}

bool InOutPanel::hasActiveControls()
{
  const bool input = ui->inputLayers->count() > 1;
  const bool output = ui->outputMode->count() > 1;
  const bool preview = ui->previewMode->count() > 1;
  return input || output || preview;
}
