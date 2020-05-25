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

namespace GmicQt
{
enum InputMode
{
  NoInput,
  Active,
  All,
  ActiveAndBelow,
  ActiveAndAbove,
  AllVisibles,
  AllInvisibles,
  AllVisiblesDesc_UNUSED,   /* Removed since 2.8.2 */
  AllInvisiblesDesc_UNUSED, /* Removed since 2.8.2 */
  AllDesc_UNUSED,           /* Removed since 2.8.2 */
  UnspecifiedInputMode = 100
};
extern const InputMode DefaultInputMode;

enum OutputMode
{
  InPlace,
  NewLayers,
  NewActiveLayers,
  NewImage,
  UnspecifiedOutputMode = 100
};
extern const OutputMode DefaultOutputMode;

enum OutputMessageMode
{
  Quiet,
  VerboseLayerName,
  VerboseConsole,
  VerboseLogFile,
  VeryVerboseConsole,
  VeryVerboseLogFile,
  DebugConsole,
  DebugLogFile,
  UnspecifiedOutputMessageMode = 100
};
extern const OutputMessageMode DefaultOutputMessageMode;

const QString & gmicVersionString();
} // namespace GmicQt

int launchPlugin();

int launchPluginHeadlessUsingLastParameters();

int launchPluginHeadless(const char * command, GmicQt::InputMode input, GmicQt::OutputMode output);

#endif // GMIC_QT_GMIC_QT_H
