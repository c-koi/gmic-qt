/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersView.h
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
#ifndef GMIC_QT_FILTERSVIEW_H
#define GMIC_QT_FILTERSVIEW_H

#include <QAction>
#include <QList>
#include <QMenu>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QString>
#include <QWidget>

namespace Ui
{
class FiltersView;
}

class FilterTreeFolder;
class FilterTreeItem;
class FilterTreeAbstractItem;
class QSettings;
class QEvent;

class FiltersView : public QWidget {
  Q_OBJECT
public:
  FiltersView(QWidget * parent);
  ~FiltersView();
  void enableModel();
  void disableModel();
  void createFolder(const QList<QString> & path);
  void addFilter(const QString & text, const QString & hash, const QList<QString> & path, bool warning);
  void addFave(const QString & text, const QString & hash);
  void selectFave(const QString & hash);
  void selectActualFilter(const QString & hash, const QList<QString> & path);
  void removeFave(const QString & hash);
  void clear();
  void sort();
  void sortFaves();
  void updateFaveItem(const QString & currentHash, const QString & newHash, const QString & newName);
  void setHeader(const QString & header);
  FilterTreeItem * selectedItem() const;
  QString selectedFilterHash() const;
  bool aFaveIsSelected() const;

  void preserveExpandedFolders();
  void restoreExpandedFolders();

  void loadSettings(const QSettings & settings);
  void saveSettings(QSettings & settings);

  void enableSelectionMode();
  void disableSelectionMode();

  void uncheckFullyUncheckedFolders();
  void adjustTreeSize();
  void expandFolders(QList<QString> & folderPaths);

  bool eventFilter(QObject * watched, QEvent * event);

signals:
  void filterSelected(QString hash);
  void faveRenamed(QString hash, QString newName);
  void faveRemovalRequested(QString hash);
  void faveAdditionRequested(QString hash);

public slots:
  void editSelectedFaveName();
  void expandAll();
  void collapseAll();
  void expandFaveFolder();
  void onCustomContextMenu(const QPoint & point);

private slots:
  void onRenameFaveFinished(QWidget * editor);
  void onReturnKeyPressedInFiltersTree();
  void onItemClicked(QModelIndex index);
  void onItemChanged(QStandardItem * item);
  void onContextMenuRemoveFave();
  void onContextMenuRenameFave();
  void onContextMenuAddFave();

private:
  FilterTreeItem * filterTreeItemFromIndex(QModelIndex index) const;
  void expandFolders(const QList<QString> & folderPaths, QStandardItem * folder);
  void uncheckFullyUncheckedFolders(QStandardItem * folder);
  void preserveExpandedFolders(QStandardItem * folder, QList<QString> & list);
  void createFaveFolder();
  void removeFaveFolder();
  void addStandardItemWithCheckbox(QStandardItem * folder, FilterTreeAbstractItem * item);
  QStandardItem * getFolderFromPath(const QList<QString> & path);
  QStandardItem * createFolder(QStandardItem * parent, QList<QString> path);
  FilterTreeItem * findFave(const QString & hash);
  static QStandardItem * getFolderFromPath(QStandardItem * parent, QList<QString> path);
  static void saveFiltersVisibility(QStandardItem * item);
  Ui::FiltersView * ui;

  QStandardItemModel _model;
  QStandardItemModel _emptyModel;
  FilterTreeFolder * _faveFolder;
  QList<QString> _cachedFolderPath;
  QStandardItem * _cachedFolder;
  QList<QString> _expandedFolderPaths;
  static const QString FilterTreePathSeparator;
  bool _isInSelectionMode;
  QMenu * _faveContextMenu;
  QMenu * _filterContextMenu;
};

#endif // GMIC_QT_FILTERSVIEW_H
