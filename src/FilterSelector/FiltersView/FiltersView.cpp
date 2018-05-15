/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersView.cpp
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
#include "FilterSelector/FiltersView/FiltersView.h"
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QSettings>
#include <QStandardItem>
#include <QStringList>
#include "Common.h"
#include "FilterSelector/FiltersView/FilterTreeFolder.h"
#include "FilterSelector/FiltersView/FilterTreeItem.h"
#include "FilterSelector/FiltersView/FilterTreeItemDelegate.h"
#include "FilterSelector/FiltersVisibilityMap.h"
#include "Globals.h"
#include "ui_filtersview.h"

const QString FiltersView::FilterTreePathSeparator("\t");

FiltersView::FiltersView(QWidget * parent) : QWidget(parent), ui(new Ui::FiltersView), _isInSelectionMode(false)
{
  ui->setupUi(this);
  ui->treeView->setModel(&_emptyModel);
  _faveFolder = 0;
  _cachedFolder = _model.invisibleRootItem();
  FilterTreeItemDelegate * delegate = new FilterTreeItemDelegate(ui->treeView);
  ui->treeView->setItemDelegate(delegate);
  ui->treeView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  ui->treeView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

  connect(delegate, SIGNAL(commitData(QWidget *)), this, SLOT(onRenameFaveFinished(QWidget *)));
  connect(ui->treeView, SIGNAL(returnKeyPressed()), this, SLOT(onReturnKeyPressedInFiltersTree()));
  connect(ui->treeView, SIGNAL(clicked(QModelIndex)), this, SLOT(onItemClicked(QModelIndex)));
  connect(&_model, SIGNAL(itemChanged(QStandardItem *)), this, SLOT(onItemChanged(QStandardItem *)));

  ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->treeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onCustomContextMenu(QPoint)));
  _faveContextMenu = new QMenu(this);
  QAction * action;
  action = _faveContextMenu->addAction(tr("Rename fave"));
  connect(action, SIGNAL(triggered(bool)), this, SLOT(onContextMenuRenameFave()));
  action = _faveContextMenu->addAction(tr("Remove fave"));
  connect(action, SIGNAL(triggered(bool)), this, SLOT(onContextMenuRemoveFave()));
  action = _faveContextMenu->addAction(tr("Clone fave"));
  connect(action, SIGNAL(triggered(bool)), this, SLOT(onContextMenuAddFave()));

  _filterContextMenu = new QMenu(this);
  action = _filterContextMenu->addAction(tr("Add fave"));
  connect(action, SIGNAL(triggered(bool)), this, SLOT(onContextMenuAddFave()));

  ui->treeView->installEventFilter(this);
}

FiltersView::~FiltersView()
{
  delete ui;
}

void FiltersView::enableModel()
{
  if (_isInSelectionMode) {
    uncheckFullyUncheckedFolders();
    _model.setHorizontalHeaderItem(1, new QStandardItem(QObject::tr("Visible")));
    _model.setColumnCount(2);
  }
  ui->treeView->setModel(&_model);
  if (_isInSelectionMode) {
    QStandardItem * headerItem = _model.horizontalHeaderItem(1);
    QString title = QString("_%1_").arg(headerItem->text());
    QFont font;
    QFontMetrics fm(font);
    int w = fm.width(title);
    ui->treeView->setColumnWidth(0, ui->treeView->width() - 2 * w);
    ui->treeView->setColumnWidth(1, w);
  }
}

void FiltersView::disableModel()
{
  ui->treeView->setModel(&_emptyModel);
}

void FiltersView::createFolder(const QList<QString> & path)
{
  createFolder(_model.invisibleRootItem(), path);
}

void FiltersView::addFilter(const QString & text, const QString & hash, const QList<QString> path, bool warning)
{
  const bool filterIsVisible = FiltersVisibilityMap::filterIsVisible(hash);
  if (!_isInSelectionMode && !filterIsVisible) {
    return;
  }
  QStandardItem * folder = getFolderFromPath(path);
  if (!folder) {
    folder = createFolder(_model.invisibleRootItem(), path);
  }
  FilterTreeItem * item = new FilterTreeItem(text);
  item->setHash(hash);
  item->setWarningFlag(warning);
  if (_isInSelectionMode) {
    addStandardItemWithCheckbox(folder, item);
    item->setVisibility(filterIsVisible);
  } else {
    folder->appendRow(item);
  }
}

void FiltersView::addFave(const QString & text, const QString & hash)
{
  const bool faveIsVisible = FiltersVisibilityMap::filterIsVisible(hash);
  if (!_isInSelectionMode && !faveIsVisible) {
    return;
  }
  if (!_faveFolder) {
    createFaveFolder();
  }
  FilterTreeItem * item = new FilterTreeItem(text);
  item->setHash(hash);
  item->setWarningFlag(false);
  item->setFaveFlag(true);
  if (_isInSelectionMode) {
    addStandardItemWithCheckbox(_faveFolder, item);
    item->setVisibility(faveIsVisible);
  } else {
    _faveFolder->appendRow(item);
  }
}

void FiltersView::selectFave(const QString & hash)
{
  // Select the fave if the model is enabled
  if (ui->treeView->model() == &_model) {
    FilterTreeItem * fave = findFave(hash);
    if (fave) {
      ui->treeView->setCurrentIndex(fave->index());
      ui->treeView->scrollTo(fave->index(), QAbstractItemView::PositionAtCenter);
    }
  }
}

void FiltersView::selectActualFilter(const QString & hash, const QList<QString> & path)
{
  QStandardItem * folder = getFolderFromPath(path);
  if (folder) {
    for (int row = 0; row < folder->rowCount(); ++row) {
      FilterTreeItem * filter = dynamic_cast<FilterTreeItem *>(folder->child(row));
      if (filter && (filter->hash() == hash)) {
        ui->treeView->setCurrentIndex(filter->index());
        ui->treeView->scrollTo(filter->index(), QAbstractItemView::PositionAtCenter);
        return;
      }
    }
  }
}

void FiltersView::removeFave(const QString & hash)
{
  FilterTreeItem * fave = findFave(hash);
  if (fave) {
    _model.removeRow(fave->row(), fave->index().parent());
    if (_faveFolder->rowCount() == 0) {
      removeFaveFolder();
    }
  }
}

void FiltersView::clear()
{
  removeFaveFolder();
  _model.invisibleRootItem()->removeRows(0, _model.invisibleRootItem()->rowCount());
  _model.setColumnCount(1);
  _cachedFolder = _model.invisibleRootItem();
  _cachedFolderPath.clear();
}

void FiltersView::sort()
{
  _model.invisibleRootItem()->sortChildren(0);
}

void FiltersView::sortFaves()
{
  if (_faveFolder) {
    _faveFolder->sortChildren(0);
  }
}

void FiltersView::updateFaveItem(const QString & currentHash, const QString & newHash, const QString & newName)
{
  FilterTreeItem * item = findFave(currentHash);
  if (!item) {
    return;
  }
  item->setText(newName);
  item->setHash(newHash);
}

void FiltersView::setHeader(const QString & header)
{
  _model.setHorizontalHeaderItem(0, new QStandardItem(header));
}

FilterTreeItem * FiltersView::selectedItem() const
{
  QModelIndex index = ui->treeView->currentIndex();
  return filterTreeItemFromIndex(index);
}

FilterTreeItem * FiltersView::filterTreeItemFromIndex(QModelIndex index) const
{
  // Get filter item even if it is the checkbox which is actually selected
  if (!index.isValid()) {
    return nullptr;
  }
  QStandardItem * item = _model.itemFromIndex(index);
  if (item) {
    int row = index.row();
    QStandardItem * parentFolder = item->parent();
    // parent is 0 for top level items
    if (!parentFolder) {
      parentFolder = _model.invisibleRootItem();
    }
    QStandardItem * leftItem = parentFolder->child(row, 0);
    if (leftItem) {
      FilterTreeItem * item = dynamic_cast<FilterTreeItem *>(leftItem);
      if (item) {
        return item;
      }
    }
  }
  return nullptr;
}

QString FiltersView::selectedFilterHash() const
{
  FilterTreeItem * item = selectedItem();
  return item ? item->hash() : QString();
}

void FiltersView::preserveExpandedFolders()
{
  if (ui->treeView->model() == &_emptyModel) {
    return;
  }
  _expandedFolderPaths.clear();
  preserveExpandedFolders(_model.invisibleRootItem(), _expandedFolderPaths);
}

void FiltersView::restoreExpandedFolders()
{
  expandFolders(_expandedFolderPaths, _model.invisibleRootItem());
}

void FiltersView::loadSettings(const QSettings &)
{
  FiltersVisibilityMap::load();
}

void FiltersView::saveSettings(QSettings & settings)
{
  if (_isInSelectionMode) {
    saveFiltersVisibility(_model.invisibleRootItem());
  }
  preserveExpandedFolders();
  settings.setValue("Config/ExpandedFolders", QStringList(_expandedFolderPaths));
  FiltersVisibilityMap::save();
}

void FiltersView::enableSelectionMode()
{
  _isInSelectionMode = true;
}

void FiltersView::disableSelectionMode()
{
  _model.setHorizontalHeaderItem(1, nullptr);
  _isInSelectionMode = false;
  saveFiltersVisibility(_model.invisibleRootItem());
}

void FiltersView::uncheckFullyUncheckedFolders()
{
  uncheckFullyUncheckedFolders(_model.invisibleRootItem());
}

void FiltersView::adjustTreeSize()
{
  ui->treeView->adjustSize();
}

void FiltersView::expandFolders(QList<QString> & folderPaths)
{
  expandFolders(folderPaths, _model.invisibleRootItem());
}

bool FiltersView::eventFilter(QObject * watched, QEvent * event)
{
  if (watched != ui->treeView) {
    return QObject::eventFilter(watched, event);
  }
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->key() == Qt::Key_Delete) {
      FilterTreeItem * item = selectedItem();
      if (item && item->isFave()) {
        QMessageBox::StandardButton button;
        button = QMessageBox::question(this, tr("Remove fave"), QString(tr("Do you really want to remove the following fave?\n\n%1\n")).arg(item->text()));
        if (button == QMessageBox::Yes) {
          emit faveRemovalRequested(item->hash());
          return true;
        }
      }
    }
  }
  return QObject::eventFilter(watched, event);
}

void FiltersView::expandFolders(const QList<QString> & folderPaths, QStandardItem * folder)
{
  int rows = folder->rowCount();
  for (int row = 0; row < rows; ++row) {
    FilterTreeFolder * subFolder = dynamic_cast<FilterTreeFolder *>(folder->child(row));
    if (subFolder) {
      if (folderPaths.contains(subFolder->path().join(FilterTreePathSeparator))) {
        ui->treeView->expand(subFolder->index());
      } else {
        ui->treeView->collapse(subFolder->index());
      }
      expandFolders(folderPaths, subFolder);
    }
  }
}

void FiltersView::editSelectedFaveName()
{
  FilterTreeItem * item = selectedItem();
  if (item && item->isFave()) {
    ui->treeView->edit(item->index());
  }
}

void FiltersView::expandAll()
{
  ui->treeView->expandAll();
}

void FiltersView::collapseAll()
{
  ui->treeView->collapseAll();
}

void FiltersView::expandFaveFolder()
{
  if (_faveFolder) {
    ui->treeView->expand(_faveFolder->index());
  }
}

void FiltersView::onCustomContextMenu(const QPoint & point)
{
  QModelIndex index = ui->treeView->indexAt(point);
  if (!index.isValid()) {
    return;
  }
  FilterTreeItem * item = filterTreeItemFromIndex(index);
  if (!item) {
    return;
  }
  onItemClicked(index);
  if (item->isFave()) {
    _faveContextMenu->exec(ui->treeView->mapToGlobal(point));
  } else {
    _filterContextMenu->exec(ui->treeView->mapToGlobal(point));
  }
}

void FiltersView::onRenameFaveFinished(QWidget * editor)
{
  QLineEdit * lineEdit = dynamic_cast<QLineEdit *>(editor);
  Q_ASSERT_X(lineEdit, "Rename Fave", "Editor is not a QLineEdit!");
  FilterTreeItem * item = selectedItem();
  if (!item) {
    return;
  }
  emit faveRenamed(item->hash(), lineEdit->text());
}

void FiltersView::onReturnKeyPressedInFiltersTree()
{
  FilterTreeItem * item = selectedItem();
  if (item) {
    emit filterSelected(item->hash());
  } else {
    QModelIndex index = ui->treeView->currentIndex();
    QStandardItem * item = _model.itemFromIndex(index);
    FilterTreeFolder * folder = item ? dynamic_cast<FilterTreeFolder *>(item) : nullptr;
    if (folder) {
      if (ui->treeView->isExpanded(index)) {
        ui->treeView->collapse(index);
      } else {
        ui->treeView->expand(index);
      }
    }
    emit filterSelected(QString());
  }
}

void FiltersView::onItemClicked(QModelIndex index)
{
  FilterTreeItem * item = filterTreeItemFromIndex(index);
  if (item) {
    emit filterSelected(item->hash());
  } else {
    emit filterSelected(QString());
  }
}

void FiltersView::onItemChanged(QStandardItem * item)
{
  if (!item->isCheckable()) {
    return;
  }
  int row = item->index().row();
  QStandardItem * parentFolder = item->parent();
  if (!parentFolder) {
    // parent is 0 for top level items
    parentFolder = _model.invisibleRootItem();
  }
  QStandardItem * leftItem = parentFolder->child(row);
  FilterTreeFolder * folder = dynamic_cast<FilterTreeFolder *>(leftItem);
  if (folder) {
    folder->applyVisibilityStatusToFolderContents();
  }
  // Force an update of the view by triggering a call of
  // QStandardItem::emitDataChanged()
  leftItem->setData(leftItem->data());
}

void FiltersView::onContextMenuRemoveFave()
{
  emit faveRemovalRequested(selectedFilterHash());
}

void FiltersView::onContextMenuRenameFave()
{
  editSelectedFaveName();
}

void FiltersView::onContextMenuAddFave()
{
  emit faveAdditionRequested(selectedFilterHash());
}

void FiltersView::uncheckFullyUncheckedFolders(QStandardItem * folder)
{
  int rows = folder->rowCount();
  for (int row = 0; row < rows; ++row) {
    FilterTreeFolder * subFolder = dynamic_cast<FilterTreeFolder *>(folder->child(row));
    if (subFolder) {
      uncheckFullyUncheckedFolders(subFolder);
      if (subFolder->isFullyUnchecked()) {
        subFolder->setVisibility(false);
      }
    }
  }
}

void FiltersView::preserveExpandedFolders(QStandardItem * folder, QList<QString> & list)
{
  int rows = folder->rowCount();
  for (int row = 0; row < rows; ++row) {
    FilterTreeFolder * subFolder = dynamic_cast<FilterTreeFolder *>(folder->child(row));
    if (subFolder) {
      if (ui->treeView->isExpanded(subFolder->index())) {
        list.push_back(subFolder->path().join(FilterTreePathSeparator));
      }
      preserveExpandedFolders(subFolder, list);
    }
  }
}

void FiltersView::createFaveFolder()
{
  if (_faveFolder) {
    return;
  }
  _faveFolder = new FilterTreeFolder(tr(FAVE_FOLDER_TEXT));
  _faveFolder->setFaveFolderFlag(true);
  _model.invisibleRootItem()->appendRow(_faveFolder);
  _model.invisibleRootItem()->sortChildren(0);
}

void FiltersView::removeFaveFolder()
{
  if (!_faveFolder) {
    return;
  }
  _model.invisibleRootItem()->removeRow(_faveFolder->row());
  _faveFolder = 0;
}

void FiltersView::addStandardItemWithCheckbox(QStandardItem * folder, FilterTreeAbstractItem * item)
{
  QList<QStandardItem *> items;
  items.push_back(item);
  QStandardItem * checkBox = new QStandardItem;
  checkBox->setCheckable(true);
  checkBox->setEditable(false);
  item->setVisibilityItem(checkBox);
  items.push_back(checkBox);
  folder->appendRow(items);
}

QStandardItem * FiltersView::getFolderFromPath(QList<QString> path)
{
  if (path == _cachedFolderPath) {
    return _cachedFolder;
  }
  _cachedFolder = getFolderFromPath(_model.invisibleRootItem(), path);
  _cachedFolderPath = path;
  return _cachedFolder;
}

QStandardItem * FiltersView::createFolder(QStandardItem * parent, QList<QString> path)
{
  Q_ASSERT_X(parent, "FiltersView", "Create folder path in null parent");
  if (path.size() == 0) {
    return parent;
  } else {
    // Look for already existing base folder in parent
    for (int row = 0; row < parent->rowCount(); ++row) {
      FilterTreeFolder * folder = dynamic_cast<FilterTreeFolder *>(parent->child(row));
      if (folder && (folder->text() == FilterTreeAbstractItem::removeWarningPrefix(path.front()))) {
        path.pop_front();
        return createFolder(folder, path);
      }
    }
    // Folder does not exist, we create it
    FilterTreeFolder * folder = new FilterTreeFolder(path.front());
    path.pop_front();
    if (_isInSelectionMode) {
      addStandardItemWithCheckbox(parent, folder);
      folder->setVisibility(true);
    } else {
      parent->appendRow(folder);
    }
    return createFolder(folder, path);
  }
}

QStandardItem * FiltersView::getFolderFromPath(QStandardItem * parent, QList<QString> path)
{
  Q_ASSERT_X(parent, "FiltersView", "Get folder path from null parent");
  if (path.isEmpty()) {
    return parent;
  }
  for (int row = 0; row < parent->rowCount(); ++row) {
    FilterTreeFolder * folder = dynamic_cast<FilterTreeFolder *>(parent->child(row));
    if (folder && (folder->text() == FilterTreeAbstractItem::removeWarningPrefix(path.front()))) {
      path.pop_front();
      return getFolderFromPath(folder, path);
    }
  }
  return 0;
}

void FiltersView::saveFiltersVisibility(QStandardItem * item)
{
  FilterTreeItem * filterItem = dynamic_cast<FilterTreeItem *>(item);
  if (filterItem) {
    FiltersVisibilityMap::setVisibility(filterItem->hash(), filterItem->isVisible());
    return;
  }
  int rows = item->rowCount();
  for (int row = 0; row < rows; ++row) {
    saveFiltersVisibility(item->child(row));
  }
}

FilterTreeItem * FiltersView::findFave(const QString & hash)
{
  const int count = _faveFolder ? _faveFolder->rowCount() : 0;
  for (int faveIndex = 0; faveIndex < count; ++faveIndex) {
    FilterTreeItem * item = static_cast<FilterTreeItem *>(_faveFolder->child(faveIndex));
    if (item->hash() == hash) {
      return item;
    }
  }
  return nullptr;
}
