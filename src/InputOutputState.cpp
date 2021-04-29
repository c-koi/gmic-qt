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
void filterObsoleteInputModes(GmicQt::InputMode & mode)
{
  switch (mode) {
  case GmicQt::AllDesc_UNUSED:
  case GmicQt::AllVisiblesDesc_UNUSED:
  case GmicQt::AllInvisiblesDesc_UNUSED:
    mode = GmicQt::UnspecifiedInputMode;
    break;
  default:
    break;
  }
}
} // namespace

namespace GmicQt
{

const InputOutputState InputOutputState::Default(GmicQt::DefaultInputMode, GmicQt::DefaultOutputMode);
const InputOutputState InputOutputState::Unspecified(GmicQt::UnspecifiedInputMode, GmicQt::UnspecifiedOutputMode);

InputOutputState::InputOutputState() : inputMode(UnspecifiedInputMode), outputMode(UnspecifiedOutputMode) {}

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
  return (inputMode == GmicQt::DefaultInputMode) && (outputMode == GmicQt::DefaultOutputMode);
}

void InputOutputState::toJSONObject(QJsonObject & object) const
{
  object = QJsonObject();
  if (inputMode != UnspecifiedInputMode) {
    object.insert("InputLayers", inputMode);
  }
  if (outputMode != DefaultOutputMode) {
    object.insert("OutputMode", outputMode);
  }
}

InputOutputState InputOutputState::fromJSONObject(const QJsonObject & object)
{
  GmicQt::InputOutputState state;
  state.inputMode = static_cast<InputMode>(object.value("InputLayers").toInt(UnspecifiedInputMode));
  filterObsoleteInputModes(state.inputMode);
  state.outputMode = static_cast<OutputMode>(object.value("OutputMode").toInt(UnspecifiedOutputMode));
  return state;
}
} // namespace GmicQt
