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

#include <QPalette>
#include <QWidget>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QList>
#include <QString>
#include <QTimer>
#include "StoredFave.h"
#include "Common.h"
#include "gmic_qt.h"

namespace Ui {
class MainWindow;
}

namespace cimg_library {
template<typename T> struct CImgList;
}

class FiltersTreeFolderItem;
class FiltersTreeFilterItem;
class FiltersTreeAbstractFilterItem;
class FiltersTreeFaveItem;
class QResizeEvent;
class Updater;
class FilterThread;

class MainWindow : public QWidget
{
  Q_OBJECT

public:
  enum PreviewPosition { PreviewOnLeft, PreviewOnRight };

  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();
  void updateFiltersFromSources(int ageLimit, bool useNetwork);

  void setDarkTheme();

public slots:
  void onUpdateDownloadsFinished(bool ok);
  void onFilterClicked(QModelIndex);
  void onApplyClicked();
  void onPreviewUpdateRequested();
  void onPreviewThreadFinished();
  void onApplyThreadFinished();
  void showWaitingCursor();
  void expandOrCollapseFolders();
  void search(QString);
  void onOkClicked();
  void onCloseClicked();
  void onCancelProcess();
  void onReset();
  void onPreviewZoomReset();
  void onUpdateFiltersClicked();
  void saveCurrentParameters();
  void onAddFave();
  void onRemoveFave();
  void onRenameFave();
  void onRenameFaveFinished(QWidget * editor);
  void onOutputMessageModeChanged(GmicQt::OutputMessageMode);
  void onToggleFullScreen(bool on);
  void onSettingsClicked();
  void startupUpdateFinished(bool);
  void onZoomIn();
  void onZoomOut();
  void showZoomWarningIfNeeded();
  void updateZoomLabel(double );
  void onFiltersSelectionModeToggled(bool);

protected:

  void timerEvent(QTimerEvent*);
  void closeEvent(QCloseEvent * e);
  void showEvent(QShowEvent *);
  void resizeEvent(QResizeEvent *);
  void saveSettings();
  void loadSettings();
  void showUpdateErrors();
  void makeConnections();
  void processImage();
  QString faveUniqueName(const QString & name, QStandardItem * toBeIgnored = nullptr);
  void activateFilter(QModelIndex index,
                      bool resetZoom,
                      const QList<QString> & values = QList<QString>() );
  FiltersTreeAbstractFilterItem * selectedFilterItem();
  void setPreviewPosition(PreviewPosition position);
  QImage buildPreviewImage(const cimg_library::CImgList<gmic_pixel_type> & images);

private slots:

  void onFiltersTreeItemChanged(QStandardItem *);

private:

  bool filtersSelectionMode();
  void clearMessage();
  void showMessage(QString text, int ms = 2000);
  void setIcons();
  bool confirmAbortProcessingOnCloseRequest();
  enum ModelType { FullModel, SelectionModel };
  FiltersTreeFolderItem * faveFolder( ModelType modelType );
  FiltersTreeFaveItem * findFave( const QString & hash, ModelType modelType );
  FiltersTreeFilterItem * findFilter( const QString & hash, ModelType modelType );
  FiltersTreeFilterItem * findFilter( const QString & hash );
  FiltersTreeAbstractFilterItem * currentTreeIndexToAbstractFilter( QModelIndex index );
  void addFaveFolder();
  void removeFaveFolder();
  void loadFaves(bool withVisibility);
  bool importFaves();
  void saveFaves();
  void buildFiltersTree();

  void backupExpandedFoldersPaths();
  void expandedFolderPaths(QStandardItem * item, QStringList & list);
  void restoreExpandedFolders();
  void restoreExpandedFolders(QStandardItem * item);

  QStringList _expandedFoldersPaths;

  QString treeIndexToPath(const QModelIndex );
  QModelIndex treePathToIndex(const QString path);
  QModelIndex treePathToIndex(const QString path, QStandardItem *);

  enum ProcessingAction { NoAction, OkAction, CloseAction, ApplyAction };

  Ui::MainWindow *ui;
  QStandardItemModel _filtersTreeModel;
  QStandardItemModel _filtersTreeModelSelection;
  QStandardItemModel * _currentFiltersTreeModel;
  FiltersTreeAbstractFilterItem * _selectedAbstractFilterItem;
  cimg_library::CImgList<float> * _gmicImages;
  FilterThread * _filterThread;
  QTimer _waitingCursorTimer;
  static const int WAITING_CURSOR_DELAY = 200;
  static const int MINIMAL_SEARCH_LENGTH = 1;
  FILE * _logFile;

  ProcessingAction _processingAction;
  PreviewPosition _previewPosition = PreviewOnRight;
  bool _darkTheme;
  bool _okButtonShouldApply = false;

  QString _lastAppliedCommand;
  QString _lastAppliedCommandArguments;
  QString _lastFilterName;
  GmicQt::OutputMessageMode _lastAppliedCommandOutputMessageMode;

  QList<StoredFave> _importedFaves;
  QList<FiltersTreeFaveItem*> _hiddenFaves;

  QIcon _expandIcon;
  QIcon _collapseIcon;
  QIcon * _expandCollapseIcon;
  int _messageTimerID;
  bool _showMaximized;
  bool _lastExecutionOK;
  bool _newSession;

  static const QString FilterTreePathSeparator;

  QVector<QWidget*> _filterUpdateWidgets;
};


#endif // _GMIC_QT_MAINWINDOW_H_
