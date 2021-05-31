/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ProgressInfoWindow.h
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
#ifndef GMIC_QT_PROGRESSINFOWINDOW_H
#define GMIC_QT_PROGRESSINFOWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QTimer>
#include "GmicQt.h"

namespace Ui
{
class ProgressInfoWindow;
}

namespace cimg_library
{
template <typename T> struct CImgList;
}

namespace GmicQt
{

class FilterThread;

class HeadlessProcessor;

class ProgressInfoWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit ProgressInfoWindow(HeadlessProcessor * processor);
  ~ProgressInfoWindow() override;

protected:
  void showEvent(QShowEvent *) override;
  void closeEvent(QCloseEvent *) override;
  void setDarkTheme();

public slots:
  void onCancelClicked(bool);
  void onProgress(float progress, int duration, unsigned long memory);
  void onInfo(QString text);
  void onProcessingFinished(const QString & errorMessage);

private:
  Ui::ProgressInfoWindow * ui;
  bool _isShown;
  HeadlessProcessor * _processor;
};

} // namespace GmicQt

#endif // GMIC_QT_PROGRESSINFOWINDOW_H
