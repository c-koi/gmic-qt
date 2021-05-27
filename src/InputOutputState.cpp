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

namespace
{
void filterDeprecatedInputMode(GmicQt::InputMode & mode)
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

const InputOutputState InputOutputState::Default(GmicQt::DefaultInputMode, GmicQt::DefaultOutputMode, GmicQt::DefaultPreviewMode);
const InputOutputState InputOutputState::Unspecified(GmicQt::UnspecifiedInputMode, GmicQt::UnspecifiedOutputMode, GmicQt::UnspecifiedPreviewMode);

InputOutputState::InputOutputState() : inputMode(UnspecifiedInputMode), outputMode(UnspecifiedOutputMode), previewMode(GmicQt::UnspecifiedPreviewMode) {}

InputOutputState::InputOutputState(InputMode inputMode, OutputMode outputMode, PreviewMode previewMode) : inputMode(inputMode), outputMode(outputMode), previewMode(previewMode) {}

bool InputOutputState::operator==(const InputOutputState & other) const
{
  return inputMode == other.inputMode && outputMode == other.outputMode && previewMode == other.previewMode;
}

bool InputOutputState::operator!=(const InputOutputState & other) const
{
  return inputMode != other.inputMode || outputMode != other.outputMode || previewMode != other.previewMode;
}

bool InputOutputState::isDefault() const
{
  return (inputMode == GmicQt::DefaultInputMode) && (outputMode == GmicQt::DefaultOutputMode) && (previewMode == GmicQt::DefaultPreviewMode);
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
  if (previewMode != DefaultPreviewMode) {
    object.insert("PreviewMode", previewMode);
  }
}

InputOutputState InputOutputState::fromJSONObject(const QJsonObject & object)
{
  GmicQt::InputOutputState state;
  state.inputMode = static_cast<InputMode>(object.value("InputLayers").toInt(UnspecifiedInputMode));
  filterDeprecatedInputMode(state.inputMode);
  state.outputMode = static_cast<OutputMode>(object.value("OutputMode").toInt(UnspecifiedOutputMode));
  state.previewMode = static_cast<PreviewMode>(object.value("PreviewMode").toInt(UnspecifiedPreviewMode));
  return state;
}
} // namespace GmicQt
