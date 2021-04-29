/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file gmic_qt.h
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
#ifndef GMIC_QT_GMIC_QT_H
#define GMIC_QT_GMIC_QT_H

#ifndef gmic_pixel_type
#define gmic_pixel_type float
#endif

#define GMIC_QT_STRINGIFY(X) #X
#define GMIC_QT_XSTRINGIFY(X) GMIC_QT_STRINGIFY(X)
#define gmic_pixel_type_str GMIC_QT_XSTRINGIFY(gmic_pixel_type)

class QString;
#include <list>
#include <string>

namespace GmicQt
{

enum UserInterfaceMode
{
  NoGUI,
  ProgressDialogGUI,
  FullGUI
};

enum InputMode
{
  NoInput,
  Active,
  All,
  ActiveAndBelow,
  ActiveAndAbove,
  AllVisible,
  AllInvisible,
  AllVisiblesDesc_UNUSED,   /* Removed since 2.8.2 */
  AllInvisiblesDesc_UNUSED, /* Removed since 2.8.2 */
  AllDesc_UNUSED,           /* Removed since 2.8.2 */
  UnspecifiedInputMode = 100
};
extern InputMode DefaultInputMode;

enum OutputMode
{
  InPlace,
  NewLayers,
  NewActiveLayers,
  NewImage,
  UnspecifiedOutputMode = 100
};
extern OutputMode DefaultOutputMode;

enum PreviewMode
{
  FirstOutput,
  SecondOutput,
  ThirdOutput,
  FourthOutput,
  First2SecondOutput,
  First2ThirdOutput,
  First2FourthOutput,
  AllOutputs,
  UnspecifiedPreviewMode = 100
};
extern PreviewMode DefaultPreviewMode;

enum OutputMessageMode
{
  Quiet,
  VerboseLayerName_UNUSED = Quiet, /* Removed since 2.9.5 */
  VerboseConsole = Quiet + 2,
  VerboseLogFile,
  VeryVerboseConsole,
  VeryVerboseLogFile,
  DebugConsole,
  DebugLogFile,
  UnspecifiedOutputMessageMode = 100
};
extern const OutputMessageMode DefaultOutputMessageMode;

const QString & gmicVersionString();

struct PluginParameters {
  std::string command;
  std::string filterPath;
  GmicQt::InputMode inputMode = GmicQt::UnspecifiedInputMode;
  GmicQt::OutputMode outputMode = GmicQt::UnspecifiedOutputMode;
  std::list<GmicQt::InputMode> disabledInputModes;
  std::list<GmicQt::OutputMode> disabledOutputModes;
  std::list<GmicQt::PreviewMode> disabledPreviewModes;
  std::string filterName() const;
};

enum PluginParameterFlag
{
  BeforeFilterExecution,
  AfterFilterExecution
};

PluginParameters lastAppliedFilterPluginParameters(PluginParameterFlag flag);

int launchPlugin(UserInterfaceMode interfaceMode = GmicQt::FullGUI, //
                 PluginParameters parameters = PluginParameters());

bool pluginDialogWasAccepted();

} // namespace GmicQt

#endif // GMIC_QT_GMIC_QT_H
