/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file MainWindow.h
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
#ifndef _GMIC_QT_MAINWINDOW_H_
#define _GMIC_QT_MAINWINDOW_H_

#include <QIcon>
#include <QList>
#include <QString>
#include <QTimer>
#include <QWidget>
#include "Common.h"
#include "Updater.h"
#include "gmic_qt.h"

namespace Ui
{
class MainWindow;
}

namespace cimg_library
{
template <typename T> struct CImgList;
}

class FiltersTreeFolderItem;
class FiltersTreeFilterItem;
class FiltersTreeAbstractFilterItem;
class FiltersTreeFaveItem;
class QResizeEvent;
class Updater;
class FilterThread;
class FiltersPresenter;

class MainWindow : public QWidget {
  Q_OBJECT

public:
  enum PreviewPosition
  {
    PreviewOnLeft,
    PreviewOnRight
  };

  explicit MainWindow(QWidget * parent = 0);
  ~MainWindow();
  void updateFiltersFromSources(int ageLimit, bool useNetwork);

  void setDarkTheme();

public slots:
  void onUpdateDownloadsFinished(int status);
  void onApplyClicked();
  void onPreviewUpdateRequested();
  void onPreviewThreadFinished();
  void onApplyThreadFinished();
  void showWaitingCursor();
  void expandOrCollapseFolders();
  void search(QString);
  void onOkClicked();
  void onCloseClicked();
  void onProgressionWidgetCancelClicked();
  void onReset();
  void onPreviewZoomReset();
  void onUpdateFiltersClicked();
  void saveCurrentParameters();
  void onAddFave();
  void onRemoveFave();
  void onRenameFave();
  void onOutputMessageModeChanged(GmicQt::OutputMessageMode);
  void onToggleFullScreen(bool on);
  void onSettingsClicked();
  void onStartupFiltersUpdateFinished(int);
  void onZoomIn();
  void onZoomOut();
  void showZoomWarningIfNeeded();
  void updateZoomLabel(double);
  void onFiltersSelectionModeToggled(bool);
  void onPreviewCheckBoxToggled(bool);
  void onFilterSelectionChanged();
  void onEscapeKeyPressed();

protected:
  void timerEvent(QTimerEvent *);
  void closeEvent(QCloseEvent * e);
  void showEvent(QShowEvent *);
  void resizeEvent(QResizeEvent *);
  void saveSettings();
  void loadSettings();
  void showUpdateErrors();
  void makeConnections();
  void processImage();
  void activateFilter(bool resetZoom);
  void setNoFilter();
  void setPreviewPosition(PreviewPosition position);
  void abortCurrentFilterThread();

  void adjustVerticalSplitter();

private slots:

  void onAbortedThreadFinished();

private:
  bool filtersSelectionMode();
  void clearMessage();
  void showMessage(QString text, int ms = 2000);
  void setIcons();
  bool confirmAbortProcessingOnCloseRequest();
  enum ModelType
  {
    FullModel,
    SelectionModel
  };
  bool askUserForGTKFavesImport();
  void buildFiltersTree();

  enum ProcessingAction
  {
    NoAction,
    OkAction,
    CloseAction,
    ApplyAction
  };

  Ui::MainWindow * ui;
  cimg_library::CImgList<float> * _gmicImages;
  FilterThread * _filterThread;
  QList<FilterThread *> _unfinishedAbortedThreads;
  QTimer _waitingCursorTimer;
  static const int WAITING_CURSOR_DELAY = 200;

  ProcessingAction _pendingActionAfterCurrentProcessing;
  PreviewPosition _previewPosition = PreviewOnRight;
  bool _okButtonShouldApply = false;

  QString _lastAppliedCommand;
  QString _lastAppliedCommandArguments;
  QString _lastFilterName;
  GmicQt::OutputMessageMode _lastAppliedCommandOutputMessageMode;

  QIcon _expandIcon;
  QIcon _collapseIcon;
  QIcon * _expandCollapseIcon;
  int _messageTimerID;
  bool _lastExecutionOK;
  bool _newSession;
  unsigned int _previewRandomSeed;

  QVector<QWidget *> _filterUpdateWidgets;
  FiltersPresenter * _filtersPresenter;
  bool _gtkFavesShouldBeImported;
};

#endif // _GMIC_QT_MAINWINDOW_H_
