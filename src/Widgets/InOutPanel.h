/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file InOutPanel.h
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
#ifndef GMIC_QT_INOUTPANEL_H
#define GMIC_QT_INOUTPANEL_H

#include <QWidget>
#include "Host/host.h"
#include "InputOutputState.h"
#include "gmic_qt.h"

namespace Ui
{
class InOutPanel;
}

class QSettings;
class QPalette;
class FilterThread;

class InOutPanel : public QWidget {
  Q_OBJECT

public:
  explicit InOutPanel(QWidget * parent = nullptr);
  ~InOutPanel();

public:
  GmicQt::InputMode inputMode() const;
  GmicQt::OutputMode outputMode() const;
  GmicQt::PreviewMode previewMode() const;
  GmicQt::OutputMessageMode outputMessageMode() const;
  void reset();

  void disableNotifications();
  void enableNotifications();
  void setInputMode(GmicQt::InputMode mode);
  void setOutputMode(GmicQt::OutputMode mode);
  void setPreviewMode(GmicQt::PreviewMode mode);

  GmicQt::InputOutputState state() const;
  void setState(const GmicQt::InputOutputState & state, bool notify);

  void setEnabled(bool);
  void disable();
  void enable();

  static void disableInputMode(GmicQt::InputMode mode);
  static void disableOutputMode(GmicQt::OutputMode mode);
  static void disablePreviewMode(GmicQt::PreviewMode mode);

  bool hasActiveControls();

signals:
  void inputModeChanged(GmicQt::InputMode);
  void previewModeChanged(GmicQt::PreviewMode);

public slots:
  void onInputModeSelected(int);
  void onOutputModeSelected(int);
  void onPreviewModeSelected(int);
  void onResetButtonClicked();
  void setDarkTheme();

private:
  static void setDefaultInputMode();
  static void setDefaultOutputMode();
  static void setDefaultPreviewMode();
  void setTopLabel();
  void updateLayoutIfUniqueRow();
  bool _notifyValueChange;
  Ui::InOutPanel * ui;
  static const int NoSelection = -1;
  static QList<GmicQt::InputMode> _enabledInputModes;
  static QList<GmicQt::OutputMode> _enabledOutputModes;
  static QList<GmicQt::PreviewMode> _enabledPreviewModes;
};

#endif // GMIC_QT_INOUTPANEL_H
