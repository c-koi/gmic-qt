/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file DialogSettings.h
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
#ifndef GMIC_QT_DIALOGSETTINGS_H
#define GMIC_QT_DIALOGSETTINGS_H

#include <QColor>
#include <QDialog>
#include <QIcon>
#include "MainWindow.h"
#include "gmic_qt.h"
class QCloseEvent;
class QSettings;

namespace Ui
{
class DialogSettings;
}

namespace GmicQt
{

enum ApplicationType
{
  GuiApplication,
  NonGuiApplication
};

class DialogSettings : public QDialog {
  Q_OBJECT

public:
  explicit DialogSettings(QWidget * parent = nullptr);
  ~DialogSettings() override;
  static MainWindow::PreviewPosition previewPosition();
  static bool logosAreVisible();
  static bool darkThemeEnabled();
  static QString languageCode();
  static bool nativeColorDialogs();
  static void saveSettings(QSettings &);
  static void loadSettings(GmicQt::ApplicationType applicationType);
  static bool previewZoomAlwaysEnabled();
  static bool notifyFailedStartupUpdate();
  static const QColor CheckBoxTextColor;
  static const QColor CheckBoxBaseColor;
  static QColor UnselectedFilterTextColor;
  static QString FolderParameterDefaultValue;
  static QString FileParameterDefaultPath;
  static int previewTimeout();
  static GmicQt::OutputMessageMode outputMessageMode();
  static QIcon AddIcon;
  static QIcon RemoveIcon;
  static QString GroupSeparator;
  static QString DecimalPoint;
  static QString NegativeSign;

public slots:
  void onRadioLeftPreviewToggled(bool);
  void onUpdateClicked();
  void onOk();
  void enableUpdateButton();
  void onDarkThemeToggled(bool);
  void onUpdatePeriodicityChanged(int i);
  void onColorDialogsToggled(bool);
  void done(int r) override;
  void onLogosVisibleToggled(bool);
  void onPreviewTimeoutChange(int);
  void onOutputMessageModeChanged(int);
  void onPreviewZoomToggled(bool);
  void onNotifyStartupUpdateFailedToggle(bool);

private:
  static void removeObsoleteKeys(QSettings &);
  Ui::DialogSettings * ui;
  static bool _darkThemeEnabled;
  static QString _languageCode;
  static bool _nativeColorDialogs;
  static MainWindow::PreviewPosition _previewPosition;
  static int _updatePeriodicity;
  static bool _logosAreVisible;
  static int _previewTimeout;
  static OutputMessageMode _outputMessageMode;
  static bool _previewZoomAlwaysEnabled;
  static bool _notifyFailedStartupUpdate;
};

} // namespace GmicQt

#endif // GMIC_QT_DIALOGSETTINGS_H
