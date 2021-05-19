/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file InputOutputState.cpp
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

#include "InputOutputState.h"
#include <QJsonObject>
#include <QString>

namespace
{

using GmicQt::InputMode;

void filterObsoleteInputModes(InputMode & mode)
{
  switch (mode) {
  case InputMode::AllDesc_UNUSED:
  case InputMode::AllVisiblesDesc_UNUSED:
  case InputMode::AllInvisiblesDesc_UNUSED:
    mode = InputMode::Unspecified;
    break;
  default:
    break;
  }
}

} // namespace

namespace GmicQt
{

const InputOutputState InputOutputState::Default(DefaultInputMode, DefaultOutputMode);
const InputOutputState InputOutputState::Unspecified(InputMode::Unspecified, OutputMode::Unspecified);

InputOutputState::InputOutputState() : inputMode(InputMode::Unspecified), outputMode(OutputMode::Unspecified) {}

InputOutputState::InputOutputState(InputMode inputMode, OutputMode outputMode) : inputMode(inputMode), outputMode(outputMode) {}

bool InputOutputState::operator==(const InputOutputState & other) const
{
  return inputMode == other.inputMode && outputMode == other.outputMode;
}

bool InputOutputState::operator!=(const InputOutputState & other) const
{
  return inputMode != other.inputMode || outputMode != other.outputMode;
}

bool InputOutputState::isDefault() const
{
  return (inputMode == DefaultInputMode) && (outputMode == DefaultOutputMode);
}

void InputOutputState::toJSONObject(QJsonObject & object) const
{
  object = QJsonObject();
  if (inputMode != InputMode::Unspecified) {
    object.insert("InputLayers", static_cast<int>(inputMode));
  }
  if (outputMode != DefaultOutputMode) {
    object.insert("OutputMode", static_cast<int>(outputMode));
  }
}

InputOutputState InputOutputState::fromJSONObject(const QJsonObject & object)
{
  InputOutputState state;
  state.inputMode = static_cast<InputMode>(object.value("InputLayers").toInt(static_cast<int>(InputMode::Unspecified)));
  filterObsoleteInputModes(state.inputMode);
  state.outputMode = static_cast<OutputMode>(object.value("OutputMode").toInt(static_cast<int>(OutputMode::Unspecified)));
  return state;
}

} // namespace GmicQt
