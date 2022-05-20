/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file Settings.h
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
#ifndef GMIC_QT_SETTINGS_H
#define GMIC_QT_SETTINGS_H

#include <QColor>
#include <QIcon>
#include <QObject>
#include "Globals.h"
#include "GmicQt.h"
#include "MainWindow.h"
class QSettings;

namespace GmicQt
{

class Settings {

public:
  Settings() = delete;
  static void setVisibleLogos(bool);
  static bool visibleLogos();
  static bool darkThemeEnabled();
  static void setDarkThemeEnabled(bool);
  static QString languageCode();
  static void setLanguageCode(const QString &);
  static bool filterTranslationEnabled();
  static void setFilterTranslationEnabled(bool);
  static MainWindow::PreviewPosition previewPosition();
  static void setPreviewPosition(MainWindow::PreviewPosition);
  static bool nativeColorDialogs();
  static void setNativeColorDialogs(bool);
  static int updatePeriodicity();
  static void setUpdatePeriodicity(int hours);
  static int previewTimeout();
  static void setPreviewTimeout(int seconds);
  static OutputMessageMode outputMessageMode();
  static void setOutputMessageMode(OutputMessageMode mode);
  static bool previewZoomAlwaysEnabled();
  static void setPreviewZoomAlwaysEnabled(bool);
  static bool notifyFailedStartupUpdate();
  static void setNotifyFailedStartupUpdate(bool);

  static void save(QSettings &);
  static void load(UserInterfaceMode userInterfaceMode);

  static const QColor CheckBoxTextColor;
  static const QColor CheckBoxBaseColor;
  static QColor UnselectedFilterTextColor;
  static QString FolderParameterDefaultValue;
  static QString FileParameterDefaultPath;
  static QIcon AddIcon;
  static QIcon RemoveIcon;
  static QString GroupSeparator;
  static QString DecimalPoint;
  static QString NegativeSign;

private:
  static void removeObsoleteKeys(QSettings &);
  static bool _visibleLogos;
  static bool _darkThemeEnabled;
  static QString _languageCode;
  static bool _filterTranslationEnabled;
  static MainWindow::PreviewPosition _previewPosition;
  static bool _nativeColorDialogs;
  static int _updatePeriodicity;
  static int _previewTimeout;
  static OutputMessageMode _outputMessageMode;
  static bool _previewZoomAlwaysEnabled;
  static bool _notifyFailedStartupUpdate;
};

} // namespace GmicQt

#endif // GMIC_QT_SETTINGS_H
