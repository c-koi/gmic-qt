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
#include <QSettings>
#include <QPalette>
#include <QDebug>
#include "ui_inoutpanel.h"
#include "InOutPanel.h"
#include "gmic.h"
#include <cstdio>

const InOutPanel::State InOutPanel::State::Unspecified(GmicQt::UnspecifiedInputMode,
                                                       GmicQt::UnspecifiedOutputMode,
                                                       GmicQt::UnspecifiedPreviewMode,
                                                       GmicQt::UnspecifiedOutputMessageMode);
/*
 * InOutPanel methods
 */

InOutPanel::InOutPanel(QWidget *parent) :
  QGroupBox(parent),
  ui(new Ui::InOutPanel)
{
  ui->setupUi(this);

  ui->inputLayers->setToolTip(tr("Input layers"));
  ui->inputLayers->addItem(tr("Input layers..."),NoSelection);
  ui->inputLayers->insertSeparator(1);
  ui->inputLayers->addItem(tr("None"),GmicQt::NoInput);
  ui->inputLayers->addItem(tr("Active (default)"),GmicQt::Active);
  ui->inputLayers->addItem(tr("All"),GmicQt::All);
  ui->inputLayers->addItem(tr("Active and below"),GmicQt::ActiveAndBelow);
  ui->inputLayers->addItem(tr("Active and above"),GmicQt::ActiveAndAbove);
  ui->inputLayers->addItem(tr("All visible"),GmicQt::AllVisibles);
  ui->inputLayers->addItem(tr("All invisible"),GmicQt::AllInvisibles);
  ui->inputLayers->addItem(tr("All visible (decr.)"),GmicQt::AllVisiblesDesc);
  ui->inputLayers->addItem(tr("All invisible (decr.)"),GmicQt::AllInvisiblesDesc);
  ui->inputLayers->addItem(tr("All (decr.)"),GmicQt::AllDesc);

  ui->outputMode->setToolTip(tr("Output mode"));
  ui->outputMode->addItem(tr("Output mode..."),NoSelection);
  ui->outputMode->insertSeparator(1);
  ui->outputMode->addItem(tr("In place (default)"),GmicQt::InPlace);
  ui->outputMode->addItem(tr("New layer(s)"),GmicQt::NewLayers);
  ui->outputMode->addItem(tr("New active layer(s)"),GmicQt::NewActiveLayers);
  ui->outputMode->addItem(tr("New image"),GmicQt::NewImage);

  ui->outputMessages->setToolTip(tr("Output messages"));
  ui->outputMessages->addItem(tr("Output messages..."),NoSelection);
  ui->outputMessages->insertSeparator(1);
  ui->outputMessages->addItem(tr("Quiet (default)"),GmicQt::Quiet);
  ui->outputMessages->addItem(tr("Verbose (layer name)"),GmicQt::VerboseLayerName);
  ui->outputMessages->addItem(tr("Verbose (console)"),GmicQt::VerboseConsole);
  ui->outputMessages->addItem(tr("Verbose (log file)"),GmicQt::VerboseLogFile);
  ui->outputMessages->addItem(tr("Very verbose (console)"),GmicQt::VeryVerboseConsole);
  ui->outputMessages->addItem(tr("Very verbose (log file)"),GmicQt::VeryVerboseLogFile);
  ui->outputMessages->addItem(tr("Debug (console)"),GmicQt::DebugConsole);
  ui->outputMessages->addItem(tr("Debug (log file)"),GmicQt::DebugLogFile);

  ui->previewMode->setToolTip(tr("Preview mode"));
  ui->previewMode->addItem(tr("Preview mode..."),NoSelection);
  ui->previewMode->insertSeparator(1);
  ui->previewMode->addItem(tr("1st ouput (default)"),GmicQt::FirstOutput);
  ui->previewMode->addItem(tr("2cd ouput"),GmicQt::SecondOutput);
  ui->previewMode->addItem(tr("3rd ouput"),GmicQt::ThirdOutput);
  ui->previewMode->addItem(tr("4th ouput"),GmicQt::FourthOutput);
  ui->previewMode->addItem(tr("1st -> 2cd ouput"),GmicQt::First2SecondOutput);
  ui->previewMode->addItem(tr("1st -> 3rd ouput"),GmicQt::First2ThirdOutput);
  ui->previewMode->addItem(tr("1st -> 4th ouput"),GmicQt::First2FourthOutput);
  ui->previewMode->addItem(tr("All ouputs"),GmicQt::AllOutputs);

  connect(ui->inputLayers,SIGNAL(currentIndexChanged(int)),
          this,SLOT(onInputModeSelected(int)));
  connect(ui->outputMode,SIGNAL(currentIndexChanged(int)),
          this,SLOT(onOutputModeSelected(int)));
  connect(ui->outputMessages,SIGNAL(currentIndexChanged(int)),
          this,SLOT(onOutputMessageSelected(int)));
  connect(ui->previewMode,SIGNAL(currentIndexChanged(int)),
          this,SLOT(onPreviewModeSelected(int)));

  _notifyValueChange = true;
}

InOutPanel::~InOutPanel()
{
  delete ui;
}

GmicQt::InputMode InOutPanel::inputMode() const
{
  int mode = ui->inputLayers->currentData().toInt();
  return (mode==NoSelection) ? GmicQt::DefaultInputMode : static_cast<GmicQt::InputMode>(mode);
}

GmicQt::OutputMode InOutPanel::outputMode() const
{
  int mode = ui->outputMode->currentData().toInt();
  return (mode==NoSelection) ? GmicQt::DefaultOutputMode : static_cast<GmicQt::OutputMode>(mode);
}

GmicQt::PreviewMode InOutPanel::previewMode() const
{
  int mode = ui->previewMode->currentData().toInt();
  return (mode==NoSelection) ? GmicQt::DefaultPreviewMode : static_cast<GmicQt::PreviewMode>(mode);
}

GmicQt::OutputMessageMode
InOutPanel::outputMessageMode() const
{
  int mode = ui->outputMessages->currentData().toInt();
  return (mode==NoSelection) ? GmicQt::DefaultOutputMessageMode : static_cast<GmicQt::OutputMessageMode>(mode);
}

QString InOutPanel::gmicEnvString() const
{
  return QString("_input_layers=%1 _output_mode=%2 _output_messages=%3 _preview_mode=%4")
      .arg(inputMode())
      .arg(outputMode())
      .arg(outputMessageMode())
      .arg(previewMode());
}

void InOutPanel::setInputMode(GmicQt::InputMode mode)
{
  int index = ui->inputLayers->findData(mode);
  ui->inputLayers->setCurrentIndex((mode == GmicQt::DefaultInputMode || index == -1) ? 0 : index);
}

void InOutPanel::setOutputMode(GmicQt::OutputMode mode)
{
  int index = ui->outputMode->findData(mode);
  ui->outputMode->setCurrentIndex((mode == GmicQt::DefaultOutputMode || index == -1) ? 0 : index);
}

void InOutPanel::setPreviewMode(GmicQt::PreviewMode mode)
{
  int index = ui->previewMode->findData(mode);
  ui->previewMode->setCurrentIndex((mode == GmicQt::DefaultPreviewMode || index == -1) ? 0 : index);
}

void InOutPanel::setOutputMessageMode(GmicQt::OutputMessageMode mode)
{
  int index = ui->outputMessages->findData(mode);
  ui->outputMessages->setCurrentIndex((mode == GmicQt::DefaultOutputMessageMode || index == -1) ? 0 : index);
}

void InOutPanel::reset()
{
  ui->outputMessages->setCurrentIndex(0);
  ui->inputLayers->setCurrentIndex(0);
  ui->outputMode->setCurrentIndex(0);
  ui->previewMode->setCurrentIndex(0);
}

void InOutPanel::onInputModeSelected(int)
{
  // TODO : Clarify this (x 4)
  //
  //  int mode = ui->inputLayers->currentData().toInt();
  //  if ( mode == NoSelection ) {
  //    ui->inputLayers->setCurrentIndex(ui->inputLayers->findData(GmicQt::DefaultInputMode));
  //  }
  if ( _notifyValueChange ) {
    // emit inputModeChanged(static_cast<GmicQt::InputMode>(ui->inputLayers->currentData().toInt()));
    emit inputModeChanged(inputMode());
  }
}

void
InOutPanel::onOutputModeSelected(int)
{
  //  int mode = ui->outputMode->currentData().toInt();
  //  if ( mode == NoSelection ) {
  //    ui->outputMode->setCurrentIndex(ui->outputMode->findData(GmicQt::DefaultOutputMode));
  //  }
}

void InOutPanel::onOutputMessageSelected(int)
{
  //  int mode = ui->outputMessages->currentData().toInt();
  //  if ( mode == NoSelection ) {
  //    ui->outputMessages->setCurrentIndex(ui->outputMessages->findData(GmicQt::DefaultOutputMessageMode));
  //  }
  if ( _notifyValueChange ) {
    //  emit outputMessageModeChanged(static_cast<GmicQt::OutputMessageMode>(mode));
    emit outputMessageModeChanged(outputMessageMode());
  }
}

void InOutPanel::onPreviewModeSelected(int)
{
  //  int mode = ui->previewMode->currentData().toInt();
  //  if ( mode == NoSelection ) {
  //    ui->previewMode->setCurrentIndex(ui->previewMode->findData(GmicQt::DefaultPreviewMode));
  //  }
  if ( _notifyValueChange ) {
    // emit previewModeChanged(static_cast<GmicQt::PreviewMode>(mode));
    emit previewModeChanged(previewMode());
  }
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
 * InOutPanel::Sate methods
 */

InOutPanel::State InOutPanel::state() const
{
  return InOutPanel::State(ui->inputLayers->currentIndex() ? inputMode() : GmicQt::UnspecifiedInputMode,
                           ui->outputMode->currentIndex() ? outputMode() : GmicQt::UnspecifiedOutputMode,
                           ui->previewMode->currentIndex() ? previewMode() : GmicQt::UnspecifiedPreviewMode,
                           ui->outputMessages->currentIndex() ? outputMessageMode() : GmicQt::UnspecifiedOutputMessageMode);
}

void InOutPanel::setState(const State & state, bool notify)
{
  bool savedNotificationStatus = _notifyValueChange;
  if ( notify ) {
    enableNotifications();
  } else {
    disableNotifications();
  }

  if ( state.inputMode != GmicQt::UnspecifiedInputMode ) {
    ui->inputLayers->setCurrentIndex(ui->inputLayers->findData(state.inputMode));
  } else {
    ui->inputLayers->setCurrentIndex(0);
  }
  if ( state.outputMode != GmicQt::UnspecifiedOutputMode ) {
    ui->outputMode->setCurrentIndex(ui->outputMode->findData(state.outputMode));
  } else {
    ui->outputMode->setCurrentIndex(0);
  }
  if ( state.outputMessageMode != GmicQt::UnspecifiedOutputMessageMode ) {
    ui->outputMessages->setCurrentIndex(ui->outputMessages->findData(state.outputMessageMode));
  } else {
    ui->outputMessages->setCurrentIndex(0);
  }
  if ( state.previewMode != GmicQt::UnspecifiedPreviewMode ) {
    ui->previewMode->setCurrentIndex(ui->previewMode->findData(state.previewMode));
  } else {
    ui->previewMode->setCurrentIndex(0);
  }

  if ( savedNotificationStatus ) {
    enableNotifications();
  } else {
    disableNotifications();
  }
}

void InOutPanel::disable()
{
  ui->inputLayers->setEnabled(false);
  ui->outputMode->setEnabled(false);
  ui->outputMessages->setEnabled(false);
  ui->previewMode->setEnabled(false);
}

void InOutPanel::enable()
{
  ui->inputLayers->setEnabled(true);
  ui->outputMode->setEnabled(true);
  ui->outputMessages->setEnabled(true);
  ui->previewMode->setEnabled(true);
}

InOutPanel::State::State()
  : inputMode(GmicQt::UnspecifiedInputMode),
    outputMode(GmicQt::UnspecifiedOutputMode),
    previewMode(GmicQt::UnspecifiedPreviewMode),
    outputMessageMode(GmicQt::UnspecifiedOutputMessageMode)
{
}

InOutPanel::State::State(GmicQt::InputMode inputMode, GmicQt::OutputMode outputMode, GmicQt::PreviewMode previewMode, GmicQt::OutputMessageMode outputMessageMode)
  : inputMode(inputMode),
    outputMode(outputMode),
    previewMode(previewMode),
    outputMessageMode(outputMessageMode)
{
}

bool InOutPanel::State::operator==( const State & other ) const
{
  return inputMode == other.inputMode
      && outputMode == other.outputMode
      && previewMode == other.previewMode
      && outputMessageMode == other.outputMessageMode;
}

bool InOutPanel::State::operator!=( const State & other ) const
{
  return inputMode != other.inputMode
      || outputMode != other.outputMode
      || previewMode != other.previewMode
      || outputMessageMode != other.outputMessageMode;
}

bool InOutPanel::State::isUnspecified() const
{
  return inputMode == GmicQt::UnspecifiedInputMode
      && outputMode == GmicQt::UnspecifiedOutputMode
      && previewMode == GmicQt::UnspecifiedPreviewMode
      && outputMessageMode == GmicQt::UnspecifiedOutputMessageMode;
}

QJsonObject InOutPanel::State::toJSONObject() const
{
  QJsonObject object;
  if ( inputMode != GmicQt::UnspecifiedInputMode ) {
    object.insert("InputLayers",inputMode);
  }
  if ( outputMode != GmicQt::UnspecifiedOutputMode ) {
    object.insert("OutputMode",outputMode);
  }
  if ( previewMode != GmicQt::UnspecifiedPreviewMode ) {
    object.insert("PreviewMode",previewMode);
  }
  if ( outputMessageMode != GmicQt::UnspecifiedOutputMessageMode ) {
    object.insert("OutputMessages",outputMessageMode);
  }
  return object;
}

InOutPanel::State InOutPanel::State::fromJSONObject(const QJsonObject & object)
{
  InOutPanel::State state;
  state.inputMode = static_cast<GmicQt::InputMode>(object.value("InputLayers").toInt(GmicQt::UnspecifiedInputMode));
  state.outputMode = static_cast<GmicQt::OutputMode>(object.value("OutputMode").toInt(GmicQt::UnspecifiedOutputMode));
  state.previewMode = static_cast<GmicQt::PreviewMode>(object.value("PreviewMode").toInt(GmicQt::UnspecifiedPreviewMode));
  state.outputMessageMode = static_cast<GmicQt::OutputMessageMode>(object.value("OutputMessages").toInt(GmicQt::UnspecifiedOutputMessageMode));
  return state;
}
