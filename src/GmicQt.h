/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file GmicQt.h
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

namespace gmic_library
{
template <typename T> struct gmic_image;
template <typename T> struct gmic_list;
} // namespace gmic_library

#ifndef gmic_pixel_type
#define gmic_pixel_type float
#endif

#define GMIC_QT_STRINGIFY(X) #X
#define GMIC_QT_XSTRINGIFY(X) GMIC_QT_STRINGIFY(X)
#define gmic_pixel_type_str GMIC_QT_XSTRINGIFY(gmic_pixel_type)

#include <list>
#include <string>
class QString;
class QImage;

namespace GmicQt
{

enum class UserInterfaceMode
{
  Silent,
  ProgressDialog,
  Full
};

enum class InputMode
{
  NoInput,
  Active,
  All,
  ActiveAndBelow,
  ActiveAndAbove,
  AllVisible,
  AllInvisible,
  AllVisiblesDesc_DEPRECATED,   /* Removed since 2.8.2 */
  AllInvisiblesDesc_DEPRECATED, /* Removed since 2.8.2 */
  AllDesc_DEPRECATED,           /* Removed since 2.8.2 */
  Unspecified = 100
};
extern InputMode DefaultInputMode;

enum class OutputMode
{
  InPlace,
  NewLayers,
  NewActiveLayers,
  NewImage,
  Unspecified = 100
};
extern OutputMode DefaultOutputMode;

enum class OutputMessageMode
{
  Quiet,
  VerboseLayerName_DEPRECATED = Quiet, /* Removed since 2.9.5 */
  VerboseConsole = Quiet + 2,
  VerboseLogFile,
  VeryVerboseConsole,
  VeryVerboseLogFile,
  DebugConsole,
  DebugLogFile,
  Unspecified = 100
};
extern const OutputMessageMode DefaultOutputMessageMode;

const QString & gmicVersionString();

extern const int GmicVersion;

struct RunParameters {
  std::string command;
  std::string filterPath;
  InputMode inputMode = InputMode::Unspecified;
  OutputMode outputMode = OutputMode::Unspecified;
  std::string filterName() const;
};

/**
 * A G'MIC filter may update the parameters it has received. This enum must be
 * used to tell the function lastAppliedFilterRunParameters() whether it
 * should return the parameters as they where supplied to the last applied
 * filter, or as they have been "returned" by this filter. If the filter does
 * not update its parameters, then "After" means the same as "Before".
 */
enum class ReturnedRunParametersFlag
{
  BeforeFilterExecution,
  AfterFilterExecution
};

RunParameters lastAppliedFilterRunParameters(ReturnedRunParametersFlag flag);

/**
 * Function that should be called to launch the plugin from the host adaptation code.
 * @return The exit status of Qt's main event loop (QApplication::exec()).
 */
int run(UserInterfaceMode interfaceMode = UserInterfaceMode::Full,                   //
        RunParameters parameters = RunParameters(),                                  //
        const std::list<InputMode> & disabledInputModes = std::list<InputMode>(),    //
        const std::list<OutputMode> & disabledOutputModes = std::list<OutputMode>(), //
        bool * dialogWasAccepted = nullptr);
/*
 * What follows may be helpful for the implementation of a host_something.cpp
 */

/**
 * Calibrate any image to fit the required number of channels (1:GRAY, 2:GRAYA, 3:RGB or 4:RGBA).
 *
 * Instantiated for T in {unsigned char, gmic_pixel_type}
 */
template <typename T> //
void calibrateImage(gmic_library::gmic_image<T> & img, const int spectrum, const bool isPreview);

void convertGmicImageToQImage(const gmic_library::gmic_image<float> & in, QImage & out);

void convertQImageToGmicImage(const QImage & in, gmic_library::gmic_image<float> & out);

} // namespace GmicQt

#endif // GMIC_QT_GMIC_QT_H
