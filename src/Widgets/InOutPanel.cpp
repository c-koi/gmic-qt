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

namespace GmicQt
{

QList<InputMode> InOutPanel::_enabledInputModes = { //
    InputMode::NoInput,                             //
    InputMode::Active,                              //
    InputMode::All,                                 //
    InputMode::ActiveAndBelow,                      //
    InputMode::ActiveAndAbove,                      //
    InputMode::AllVisible,                          //
    InputMode::AllInvisible};

QList<OutputMode> InOutPanel::_enabledOutputModes = { //
    OutputMode::InPlace,                              //
    OutputMode::NewLayers,                            //
    OutputMode::NewActiveLayers,                      //
    OutputMode::NewImage};

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
  ui->inputLayers->addItem(TEXT, static_cast<int>(MODE))

  ADD_INPUT_IF_ENABLED(InputMode::NoInput, tr("None"));
  ADD_INPUT_IF_ENABLED(InputMode::Active, tr("Active (default)"));
  ADD_INPUT_IF_ENABLED(InputMode::All, tr("All"));
  ADD_INPUT_IF_ENABLED(InputMode::ActiveAndBelow, tr("Active and below"));
  ADD_INPUT_IF_ENABLED(InputMode::ActiveAndAbove, tr("Active and above"));
  ADD_INPUT_IF_ENABLED(InputMode::AllVisible, tr("All visible"));
  ADD_INPUT_IF_ENABLED(InputMode::AllInvisible, tr("All invisible"));
  // "decr." input mode have been removed (since 2.8.2)
  //  ui->inputLayers->addItem(tr("All visible (decr.)"), AllVisiblesDesc);
  //  ui->inputLayers->addItem(tr("All invisible (decr.)"), AllInvisiblesDesc);
  //  ui->inputLayers->addItem(tr("All (decr.)"), AllDesc);

  if (ui->inputLayers->count() == 1) {
    ui->labelInputLayers->hide();
    ui->inputLayers->hide();
  }

  ui->outputMode->setToolTip(tr("Output mode"));

#define ADD_OUTPUT_IF_ENABLED(MODE, TEXT)                                                                                                                                                              \
  if (_enabledOutputModes.contains(MODE))                                                                                                                                                              \
  ui->outputMode->addItem(TEXT, static_cast<int>(MODE))

  ADD_OUTPUT_IF_ENABLED(OutputMode::InPlace, tr("In place (default)"));
  ADD_OUTPUT_IF_ENABLED(OutputMode::NewLayers, tr("New layer(s)"));
  ADD_OUTPUT_IF_ENABLED(OutputMode::NewActiveLayers, tr("New active layer(s)"));
  ADD_OUTPUT_IF_ENABLED(OutputMode::NewImage, tr("New image"));

  if (ui->outputMode->count() == 1) {
    ui->labelOutputMode->hide();
    ui->outputMode->hide();
  }

  setTopLabel();
  updateLayoutIfUniqueRow();

  connect(ui->inputLayers, SIGNAL(currentIndexChanged(int)), this, SLOT(onInputModeSelected(int)));
  connect(ui->outputMode, SIGNAL(currentIndexChanged(int)), this, SLOT(onOutputModeSelected(int)));
  connect(ui->tbReset, SIGNAL(clicked(bool)), this, SLOT(onResetButtonClicked()));

  _notifyValueChange = true;
}

InOutPanel::~InOutPanel()
{
  delete ui;
}

InputMode InOutPanel::inputMode() const
{
  int mode = ui->inputLayers->currentData().toInt();
  return static_cast<InputMode>(mode);
}

OutputMode InOutPanel::outputMode() const
{
  int mode = ui->outputMode->currentData().toInt();
  return static_cast<OutputMode>(mode);
}

void InOutPanel::setInputMode(InputMode mode)
{
  int index = ui->inputLayers->findData(static_cast<int>(mode));
  ui->inputLayers->setCurrentIndex((index == -1) ? ui->inputLayers->findData(static_cast<int>(DefaultInputMode)) : index);
}

void InOutPanel::setOutputMode(OutputMode mode)
{
  int index = ui->outputMode->findData(static_cast<int>(mode));
  ui->outputMode->setCurrentIndex((index == -1) ? ui->outputMode->findData(static_cast<int>(DefaultOutputMode)) : index);
}

void InOutPanel::reset()
{
  ui->inputLayers->setCurrentIndex(ui->inputLayers->findData(static_cast<int>(DefaultInputMode)));
  ui->outputMode->setCurrentIndex(ui->outputMode->findData(static_cast<int>(DefaultOutputMode)));
}

void InOutPanel::onInputModeSelected(int)
{
  if (_notifyValueChange) {
    emit inputModeChanged(inputMode());
  }
}

void InOutPanel::onOutputModeSelected(int) {}

void InOutPanel::onResetButtonClicked()
{
  setState(InputOutputState::Default, true);
}

void InOutPanel::setDarkTheme()
{
  ui->tbReset->setIcon(LOAD_ICON("view-refresh"));
}

void InOutPanel::setDefaultInputMode()
{
  if (_enabledInputModes.contains(DefaultInputMode)) {
    return;
  }
  const int start = static_cast<int>(InputMode::Active);
  const int stop = static_cast<int>(InputMode::AllInvisible);
  for (int m = start; m <= stop; ++m) {
    const auto mode = static_cast<InputMode>(m);
    if (_enabledInputModes.contains(mode)) {
      DefaultInputMode = mode;
      return;
    }
  }
  Q_ASSERT_X(_enabledInputModes.contains(InputMode::NoInput), __FUNCTION__, "No input mode left by host settings. Default mode cannot be determined.");
  DefaultInputMode = InputMode::NoInput;
}

void InOutPanel::setDefaultOutputMode()
{
  if (_enabledOutputModes.contains(DefaultOutputMode)) {
    return;
  }
  Q_ASSERT_X(!_enabledOutputModes.isEmpty(), __FUNCTION__, "No output mode left by host settings. Default mode cannot be determined.");
  const int start = (int)OutputMode::InPlace;
  const int stop = (int)OutputMode::NewImage;
  for (int m = start; m <= stop; ++m) {
    const auto mode = static_cast<OutputMode>(m);
    if (_enabledOutputModes.contains(mode)) {
      DefaultOutputMode = mode;
      return;
    }
  }
}

void InOutPanel::setTopLabel()
{
  const bool input = ui->inputLayers->count() > 1;
  const bool output = ui->outputMode->count() > 1;
  if (input && output) {
    ui->topLabel->setText(tr("Input / Output"));
  } else if (input) {
    ui->topLabel->setText(tr("Input"));
  } else if (output) {
    ui->topLabel->setText(tr("Output"));
  }
}

void InOutPanel::updateLayoutIfUniqueRow()
{
  const bool input = ui->inputLayers->count() > 1;
  const bool output = ui->outputMode->count() > 1;
  if ((input + output) > 1) {
    return;
  }
  if (input) {
    ui->topLabel->setText(ui->labelInputLayers->text());
    ui->horizontalLayout->insertWidget(1, ui->inputLayers);
  } else if (output) {
    ui->topLabel->setText(ui->labelOutputMode->text());
    ui->horizontalLayout->insertWidget(1, ui->outputMode);
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

InputOutputState InOutPanel::state() const
{
  return {inputMode(), outputMode()};
}

void InOutPanel::setState(const InputOutputState & state, bool notify)
{
  bool savedNotificationStatus = _notifyValueChange;
  if (notify) {
    enableNotifications();
  } else {
    disableNotifications();
  }

  setInputMode(state.inputMode);
  setOutputMode(state.outputMode);

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
}

void InOutPanel::disable()
{
  setEnabled(false);
}

void InOutPanel::enable()
{
  setEnabled(true);
}

void InOutPanel::disableInputMode(InputMode mode)
{
  const bool isDefault = (mode == DefaultInputMode);
  _enabledInputModes.removeOne(mode);
  if (isDefault) {
    setDefaultInputMode();
  }
}

void InOutPanel::disableOutputMode(OutputMode mode)
{
  const bool isDefault = (mode == DefaultOutputMode);
  _enabledOutputModes.removeOne(mode);
  if (isDefault) {
    setDefaultOutputMode();
  }
}

bool InOutPanel::hasActiveControls()
{
  const bool input = ui->inputLayers->count() > 1;
  const bool output = ui->outputMode->count() > 1;
  return input || output;
}

} // namespace GmicQt
