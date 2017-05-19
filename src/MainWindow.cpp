/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file MainWindow.cpp
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
#include <QCursor>
#include <QDebug>
#include <QAction>
#include <QSettings>
#include <QTextDocument>
#include <QTextStream>
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>
#include <QList>
#include <QFileInfo>
#include <QKeySequence>
#include <QAbstractItemModel>
#include <QShowEvent>
#include <QPainter>
#include <QDesktopWidget>
#include <DialogSettings.h>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QStyleFactory>
#include <QJsonDocument>
#include <QJsonArray>
#include <iostream>
#include <typeinfo>
#include <cassert>
#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "Common.h"
#include "FilterThread.h"
#include "ImageConverter.h"
#include "ParametersCache.h"
#include "FiltersTreeAbstractFilterItem.h"
#include "FiltersTreeItemDelegate.h"
#include "FiltersTreeFilterItem.h"
#include "FiltersTreeFolderItem.h"
#include "FiltersTreeFaveItem.h"
#include "FiltersVisibilityMap.h"
#include "Updater.h"
#include "GmicStdlibParser.h"
#include "ImageTools.h"
#include "LayersExtentProxy.h"
#include "host.h"
#include "gmic.h"

//
// TODO : Handle window maximization properly (Windows as well as some Linux desktops)
//

const QString MainWindow::FilterTreePathSeparator("\t");

MainWindow::MainWindow(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::MainWindow),
  _gmicImages(new cimg_library::CImgList<gmic_pixel_type>),
  _filterThread(0)
{
  ui->setupUi(this);
  _logFile = 0;
  _messageTimerID = 0;

#ifdef gmic_prerelease
#define BETA_SUFFIX "_pre#" gmic_prerelease
#else
#define BETA_SUFFIX ""
#endif

  setWindowTitle(QString("G'MIC-Qt %1 - %2.%3.%4" BETA_SUFFIX )
                 .arg(GmicQt::HostApplicationName.isEmpty() ? QString() : QString("for %1 ").arg(GmicQt::HostApplicationName))
                 .arg(gmic_version/100).arg((gmic_version/10)%10).arg(gmic_version%10));

  QStringList tsp = QIcon::themeSearchPaths();
  tsp.append(QString("/usr/share/icons/gnome"));
  QIcon::setThemeSearchPaths(tsp);

  ui->tbZoomIn->setToolTip("Zoom in");
  ui->tbZoomOut->setToolTip("Zoom out");
  ui->tbZoomReset->setToolTip("Reset zoom");

  ui->labelWarning->setToolTip("Warning: Preview may be inaccurate (zoom factor has been modified)");
  ui->tbAddFave->setToolTip(tr("Add fave"));

  ui->tbResetParameters->setToolTip(tr("Reset parameters to default values"));
  ui->tbResetParameters->setVisible(false);

  ui->tbUpdateFilters->setToolTip(tr("Update filters"));

  ui->tbRenameFave->setToolTip(tr("Rename fave"));
  ui->tbRenameFave->setEnabled(false);
  ui->tbRemoveFave->setToolTip(tr("Remove fave"));
  ui->tbRemoveFave->setEnabled(false);
  ui->pbFullscreen->setCheckable(true);
  ui->pbCancel->setShortcut(QKeySequence(Qt::Key_Escape));
  ui->tbExpandCollapse->setToolTip(tr("Expand/Collapse all"));

  ui->logosLabel->setToolTip(tr("G'MIC (http://gmic.eu)<br/>"
                                "GREYC (http://www.greyc.fr)<br/>"
                                "CNRS (http://www.cnrs.fr)<br/>"
                                "Normandy University (http://www.unicaen.fr)<br/>"
                                "Ensicaen (http://www.ensicaen.fr)"));
  ui->logosLabel->setPixmap(QPixmap(":resources/logos.png"));

  _waitingCursorTimer.setSingleShot(true);
  connect(&_waitingCursorTimer,SIGNAL(timeout()),
          this,SLOT(showWaitingCursor()));

  ui->cbPreview->setChecked(true);

  _currentFiltersTreeModel = & _filtersTreeModel;

  ui->filterName->setTextFormat(Qt::RichText);
  ui->filterName->setVisible(false);

  ui->progressInfoWidget->hide();
  ui->messageLabel->setText(QString());
  ui->messageLabel->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);

  ui->filterParams->setNoFilter();
  _processingAction = NoAction;
  ui->splitter->setChildrenCollapsible(false);

  FiltersTreeItemDelegate * delegate = new FiltersTreeItemDelegate(ui->filtersTree);
  ui->filtersTree->setItemDelegate(delegate);
  connect(delegate,SIGNAL(commitData(QWidget*)),
          this,SLOT(onRenameFaveFinished(QWidget*)));

  QAction * searchAction = new QAction(this);
  searchAction->setShortcut(QKeySequence::Find);
  searchAction->setShortcutContext(Qt::ApplicationShortcut);
  connect(searchAction,SIGNAL(triggered(bool)),
          ui->searchField,SLOT(setFocus()));
  addAction(searchAction);

  QFont font;
  QFontMetrics fm(font);
  ui->frame_zoom_level->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  ui->labelZoomLevel->setMinimumWidth(fm.width("88.88 %")+10);
  ui->labelZoomLevel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

  QPalette p = qApp->palette();
  DialogSettings::UnselectedFilterTextColor = p.color(QPalette::Disabled,QPalette::WindowText);

  loadSettings();

  ParametersCache::load(!_newSession);

  setIcons();

  LayersExtentProxy::clearCache();
  QSize layersExtents = LayersExtentProxy::getExtent(ui->inOutSelector->inputMode());
  ui->previewWidget->setFullImageSize(layersExtents);
  makeConnections();
}

MainWindow::~MainWindow()
{
  saveCurrentParameters();
  saveFaves();
  ParametersCache::save();

  // Save visibility
  if ( filtersSelectionMode() ) {
    GmicStdLibParser::saveFiltersVisibility(_filtersTreeModel.invisibleRootItem());
  }
  FiltersVisibilityMap::save();

  saveSettings();
  if ( _logFile ) {
    fclose(_logFile);
  }
  delete ui;
  delete _gmicImages;
  qDeleteAll(_hiddenFaves);
  _hiddenFaves.clear();
}

void MainWindow::setIcons()
{
  ui->tbRenameFave->setIcon(QIcon(":/resources/rename.png"));
  ui->pbSettings->setIcon(LOAD_ICON("package_settings"));
  ui->pbFullscreen->setIcon(LOAD_ICON("view-fullscreen"));
  ui->tbZoomOut->setIcon(LOAD_ICON("zoom-out"));
  ui->tbZoomIn->setIcon(LOAD_ICON("zoom-in"));
  ui->tbZoomReset->setIcon(LOAD_ICON("view-refresh"));
  ui->tbUpdateFilters->setIcon(LOAD_ICON("view-refresh"));
  ui->pbApply->setIcon(LOAD_ICON("system-run"));
  ui->pbOk->setIcon(LOAD_ICON("insert-image"));
  ui->tbResetParameters->setIcon(LOAD_ICON("view-refresh"));
  ui->pbCancel->setIcon(LOAD_ICON("process-stop"));
  ui->tbAddFave->setIcon(LOAD_ICON("bookmark-add"));
  ui->tbRemoveFave->setIcon(LOAD_ICON("bookmark-remove"));
  _expandIcon = LOAD_ICON("draw-arrow-down");
  _collapseIcon = LOAD_ICON("draw-arrow-up");
  _expandCollapseIcon = &_expandIcon;
  ui->tbExpandCollapse->setIcon(_expandIcon);

  ui->labelWarning->setPixmap(QPixmap(":/images/no_warning.png"));
}

void MainWindow::setDarkTheme()
{
  // SHOW(QStyleFactory::keys());
  qApp->setStyle(QStyleFactory::create("Fusion"));
  QPalette p = qApp->palette();
  p.setColor(QPalette::Window, QColor(53,53,53));
  p.setColor(QPalette::Button, QColor(73,73,73));
  p.setColor(QPalette::Highlight, QColor(110,110,110));
  p.setColor(QPalette::Text, QColor(255,255,255));
  p.setColor(QPalette::ButtonText, QColor(255,255,255));
  p.setColor(QPalette::WindowText, QColor(255,255,255));
  QColor linkColor(100,100,100);
  linkColor = linkColor.lighter();
  p.setColor(QPalette::Link, linkColor);
  p.setColor(QPalette::LinkVisited, linkColor);
  qApp->setPalette(p);

  p = ui->cbInternetUpdate->palette();
  p.setColor(QPalette::Text, DialogSettings::CheckBoxTextColor);
  p.setColor(QPalette::Base, DialogSettings::CheckBoxBaseColor);
  ui->cbInternetUpdate->setPalette(p);
  ui->cbPreview->setPalette(p);
  ui->cbFiltersSelectionMode->setPalette(p);

  const QString css = "QTreeView { background: #505050; }"
                      "QLineEdit { background: #505050; }"
                      "QTextEdit { background: #505050; }"
                      "QSpinBox  { background: #505050; }"
                      "QDoubleSpinBox { background: #505050; }";
  qApp->setStyleSheet( css );

  //DialogSettings::UnselectedFilterTextColor.setRgb(255,0,0);
  DialogSettings::UnselectedFilterTextColor = DialogSettings::UnselectedFilterTextColor.darker(150);
}

void MainWindow::updateFiltersFromSources(int ageLimit, bool useNetwork)
{
  connect(Updater::getInstance(),SIGNAL(downloadsFinished(bool)),
          this,SLOT(onUpdateDownloadsFinished(bool)),
          Qt::UniqueConnection);
  Updater::getInstance()->startUpdate(ageLimit,60,useNetwork);
}

void MainWindow::onUpdateDownloadsFinished(bool ok)
{
  if ( !ok ) {
    showUpdateErrors();
  } else {
    if ( ui->cbInternetUpdate->isChecked() ) {
      QMessageBox::information(this,tr("Update completed"),tr("Filter definitions have been updated."));
    } else {
      showMessage(tr("Filter definitions have been updated."),3000);
    }
  }
  buildFiltersTree();
  ui->filtersTree->update();
  ui->tbUpdateFilters->setEnabled(true);
  if ( selectedFilterItem() ) {
    ui->previewWidget->sendUpdateRequest();
  }
}

void MainWindow::buildFiltersTree()
{
  // Save current expand/collapse status
  backupExpandedFoldersPaths();
  QString currentHash = selectedFilterItem() ? selectedFilterItem()->hash() : QString();
  if ( !currentHash.isEmpty() ) {
    saveCurrentParameters();
  }
  GmicStdLibParser::GmicStdlib = Updater::getInstance()->buildFullStdlib();
  _filtersTreeModel.clear();
  _filtersTreeModelSelection.clear();
  const bool withVisibility = filtersSelectionMode();
  GmicStdLibParser::buildFiltersTree(ui->filtersTree,
                                     _filtersTreeModel,
                                     withVisibility);
  loadFaves(withVisibility);
  _filtersTreeModel.invisibleRootItem()->sortChildren(0);

  // Select previously selected filter
  if ( !currentHash.isEmpty() ) {
    FiltersTreeFilterItem * item = findFilter(currentHash,FullModel);
    if ( item ) {
      ui->filtersTree->setCurrentIndex(item->index());
      ui->filtersTree->scrollTo(item->index(),QAbstractItemView::PositionAtCenter);
      selectFilter(item->index(),false);
    }
  }

  if ( withVisibility ) {
    // Adjust column sizes
    QStandardItem * headerItem = _filtersTreeModel.horizontalHeaderItem(1);
    Q_ASSERT_X(headerItem,"MainWindow::buildFiltersTree()","Missing header item");
    QString title = QString("_%1_").arg(headerItem->text());
    QFont font;
    QFontMetrics fm(font);
    int w = fm.width(title);
    ui->filtersTree->setColumnWidth(0, ui->filtersTree->width() - w * 2);
    ui->filtersTree->setColumnWidth(1, w);
    GmicStdLibParser::uncheckFullyUncheckedFolders(_filtersTreeModel.invisibleRootItem());
  } else {
    // Remove empty folders
    do {
      // Nothing
    } while (GmicStdLibParser::cleanupFolders(_filtersTreeModel.invisibleRootItem()));
  }
  restoreExpandedFolders();
}

void MainWindow::startupUpdateFinished(bool ok)
{
  QObject::disconnect(Updater::getInstance(),SIGNAL(downloadsFinished(bool)),
                      this,SLOT(startupUpdateFinished(bool)));
  if ( _showMaximized  ) {
    show();
    showMaximized();
  } else {
    show();
  }
  if ( !ok ) {
    showUpdateErrors();
  } else if (Updater::getInstance()->someNetworkUpdateAchieved() ) {
    showMessage(tr("Filter definitions have been updated"),4000);
  }
}

void MainWindow::onZoomIn()
{
  ui->previewWidget->zoomIn();
}

void MainWindow::onZoomOut()
{
  ui->previewWidget->zoomOut();
}

void MainWindow::showZoomWarningIfNeeded()
{
  FiltersTreeAbstractFilterItem * item = selectedFilterItem();
  if ( item && !item->isAccurateIfZoomed() && !ui->previewWidget->isAtDefaultZoom() ) {
    ui->labelWarning->setPixmap(QPixmap(":/images/warning.png"));
  } else {
    ui->labelWarning->setPixmap(QPixmap(":/images/no_warning.png"));
  }
}

void MainWindow::updateZoomLabel()
{
  double zoom = ui->previewWidget->currentZoomFactor();
  bool decimals = static_cast<int>(zoom*10000) % 100 ;
  QString text;
  if (decimals && zoom < 1) {
    text = QString("%L1 %").arg(zoom*100.0,0,'f',2);
  } else {
    text = QString("%1 %").arg(static_cast<int>(zoom*100));
  }
  ui->labelZoomLevel->setText(text);
}

void MainWindow::onFiltersSelectionModeToggled(bool on)
{
  if ( ! on ) {
    GmicStdLibParser::saveFiltersVisibility(_filtersTreeModel.invisibleRootItem());
  }
  buildFiltersTree();
}

void MainWindow::clearMessage()
{
  if ( !_messageTimerID ) {
    return;
  }
  killTimer(_messageTimerID);
  ui->messageLabel->setText(QString());
  _messageTimerID = 0;
}

void MainWindow::timerEvent(QTimerEvent * e)
{
  if ( e->timerId() == _messageTimerID ) {
    clearMessage();
    e->accept();
  }
  e->ignore();
}

void MainWindow::showMessage(QString text, int ms)
{
  clearMessage();
  ui->messageLabel->setText(text);
  _messageTimerID = startTimer(ms);
}

void MainWindow::showUpdateErrors()
{
  QString message(tr("The update could not be achieved<br>"
                     "because of the following errors:<br>"));
  QList<QString> errors = Updater::getInstance()->errorMessages();
  for ( QString s : errors) {
    message += QString("<br/>%1").arg(s);
  }
  QMessageBox::information(this,tr("Update error"),message);
}


void MainWindow::makeConnections()
{
  connect(ui->previewWidget,SIGNAL(zoomChanged()),
          this,SLOT(showZoomWarningIfNeeded()));
  connect(ui->previewWidget,SIGNAL(zoomChanged()),
          this,SLOT(updateZoomLabel()));

  connect(ui->filtersTree,SIGNAL(clicked(QModelIndex)),
          this,SLOT(onFilterClicked(QModelIndex)));

  connect(ui->pbOk,SIGNAL(clicked(bool)),
          this,SLOT(onOkClicked()));
  connect(ui->pbCancel,SIGNAL(clicked(bool)),
          this,SLOT(onCloseClicked()));
  connect(ui->pbApply,SIGNAL(clicked(bool)),
          this,SLOT(onApplyClicked()));
  connect(ui->tbResetParameters,SIGNAL(clicked(bool)),
          this,SLOT(onReset()));

  connect(ui->tbUpdateFilters,SIGNAL(clicked(bool)),
          this,SLOT(onUpdateFiltersClicked()));

  connect(ui->pbSettings,SIGNAL(clicked(bool)),
          this,SLOT(onSettingsClicked()));

  connect(ui->pbFullscreen,SIGNAL(toggled(bool)),
          this,SLOT(onToggleFullScreen(bool)));

  connect(ui->filterParams,SIGNAL(valueChanged()),
          ui->previewWidget,SLOT(sendUpdateRequest()));

  connect(ui->previewWidget,SIGNAL(previewUpdateRequested()),
          this,SLOT(onPreviewUpdateRequested()));

  connect(ui->tbZoomIn,SIGNAL(clicked(bool)),
          this,SLOT(onZoomIn()));
  connect(ui->tbZoomOut,SIGNAL(clicked(bool)),
          this,SLOT(onZoomOut()));
  connect(ui->tbZoomReset,SIGNAL(clicked(bool)),
          this,SLOT(onPreviewZoomReset()));

  connect(ui->tbAddFave,SIGNAL(clicked(bool)),
          this,SLOT(onAddFave()));
  connect(ui->tbRemoveFave,SIGNAL(clicked(bool)),
          this,SLOT(onRemoveFave()));
  connect(ui->tbRenameFave,SIGNAL(clicked(bool)),
          this,SLOT(onRenameFave()));

  connect(ui->inOutSelector,SIGNAL(inputModeChanged(GmicQt::InputMode)),
          ui->previewWidget,SLOT(sendUpdateRequest()));
  connect(ui->inOutSelector,SIGNAL(outputMessageModeChanged(GmicQt::OutputMessageMode)),
          this,SLOT(onOutputMessageModeChanged(GmicQt::OutputMessageMode)));
  connect(ui->inOutSelector,SIGNAL(previewModeChanged(GmicQt::PreviewMode)),
          ui->previewWidget,SLOT(sendUpdateRequest()));

  connect(ui->cbPreview,SIGNAL(toggled(bool)),
          ui->previewWidget,SLOT(enablePreview(bool)));
  connect(ui->searchField,SIGNAL(textChanged(QString)),
          this,SLOT(search(QString)));

  connect(ui->tbExpandCollapse,SIGNAL(clicked(bool)),
          this,SLOT(expandOrCollapseFolders()));
  connect(ui->progressInfoWidget,SIGNAL(cancel()),
          this,SLOT(onCancelProcess()));

  connect(ui->cbFiltersSelectionMode,SIGNAL(toggled(bool)),
          this,SLOT(onFiltersSelectionModeToggled(bool)));

  connect(&_filtersTreeModel,SIGNAL(itemChanged(QStandardItem*)),
          this,SLOT(onFiltersTreeItemChanged(QStandardItem*)));
}

void
MainWindow::onFilterClicked(QModelIndex index)
{
  selectFilter(index,false);
  ui->previewWidget->sendUpdateRequest();
}

void
MainWindow::onPreviewUpdateRequested()
{
  if ( ! ui->cbPreview->isChecked() ) {
    ui->previewWidget->invalidateSavedPreview();
    return;
  }

  if ( _filterThread ) {
    _filterThread->disconnect(this);
    _filterThread->abortGmic();
    _filterThread = 0;
  }

  if ( !selectedFilterItem() || ui->filterParams->previewCommand().isEmpty() || ui->filterParams->previewCommand() == "_none_" ) {
    ui->previewWidget->displayOriginalImage();
  } else {
    _gmicImages->assign(1);
    double x,y,w,h;
    ui->previewWidget->normalizedVisibleRect(x,y,w,h);
    GmicQt::InputMode inputMode = ui->inOutSelector->inputMode();
    gmic_list<char> imageNames;
    gmic_qt_get_cropped_images(*_gmicImages,imageNames,x,y,w,h,inputMode);
    ui->previewWidget->updateImageNames(imageNames,inputMode);
    double zoomFactor = ui->previewWidget->currentZoomFactor();
    if ( zoomFactor < 1.0 ) {
      QImage qimage;
      QImage scaled;
      for ( unsigned int i = 0; i < _gmicImages->size(); ++i ) {
        gmic_image<float> & image = (*_gmicImages)[i];
        ImageConverter::convert(image,qimage);
        scaled = qimage.scaled(image.width()*zoomFactor,
                               image.height()*zoomFactor);
        ImageConverter::convert(scaled,image);
        //  image.resize(image.width()*zoomFactor,image.height()*zoomFactor,1,-100,1);
      }
    }
    QString env = ui->inOutSelector->gmicEnvString();
    env += QString(" _preview_width=%1 _preview_height=%2")
        .arg(ui->previewWidget->width())
        .arg(ui->previewWidget->height());
    _filterThread = new FilterThread(this,
                                     selectedFilterItem()->plainText(),
                                     ui->filterParams->previewCommand(),
                                     ui->filterParams->valueString(),
                                     env,
                                     ui->inOutSelector->outputMessageMode());
    _filterThread->setInputImages(*_gmicImages,imageNames);
    connect(_filterThread,SIGNAL(finished()),
            this,SLOT(onPreviewThreadFinished()));
    connect(_filterThread,SIGNAL(finished()),
            _filterThread,SLOT(deleteLater()));
    _waitingCursorTimer.start(WAITING_CURSOR_DELAY);
    _okButtonShouldApply = true;
    _filterThread->start();
  }
}

void MainWindow::onPreviewThreadFinished()
{
  if ( !_filterThread ) {
    return;
  }
  QStringList list = GmicStdLibParser::parseStatus(_filterThread->gmicStatus());
  if ( ! list.isEmpty() ) {
    ui->filterParams->setValues(list,false);
  }
  if ( _filterThread->failed() ) {
    QImage image = ui->previewWidget->image();
    image = image.scaled(ui->previewWidget->width(),
                         ui->previewWidget->height(),
                         Qt::KeepAspectRatio);
    QPainter painter(&image);
    painter.fillRect(image.rect(),QColor(40,40,40,200));
    painter.setPen(Qt::green);
    painter.drawText(image.rect(),
                     Qt::AlignCenter|Qt::TextWordWrap,
                     _filterThread->errorMessage());
    painter.end();
    ui->previewWidget->setPreviewImage(image);
  } else {
    gmic_list<gmic_pixel_type> images = _filterThread->images();
    for (unsigned int i = 0; i < images.size(); ++i) {
      gmic_qt_apply_color_profile(images[i]);
    }
    ui->previewWidget->setPreviewImage(buildPreviewImage(images));
  }

  if ( QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::WaitCursor ) {
    QApplication::restoreOverrideCursor();
  }
  _waitingCursorTimer.stop();
  ui->previewWidget->savePreview();
  _filterThread = 0;
}

void
MainWindow::processImage()
{
  // Abort any already running thread
  if ( _filterThread ) {
    _filterThread->disconnect(this);
    _filterThread->abortGmic();
    _filterThread = 0;
  }
  if ( !selectedFilterItem() || ui->filterParams->command().isEmpty() || ui->filterParams->command() == "_none_" ) {
    return;
  }
  _gmicImages->assign();
  gmic_list<char> imageNames;
  gmic_qt_get_cropped_images(*_gmicImages,imageNames,-1,-1,-1,-1,ui->inOutSelector->inputMode());
  _filterThread = new FilterThread(this,
                                   _lastFilterName = selectedFilterItem()->plainText(),
                                   _lastAppliedCommand = ui->filterParams->command(),
                                   _lastAppliedCommandArguments = ui->filterParams->valueString(),
                                   ui->inOutSelector->gmicEnvString(),
                                   _lastAppliedCommandOutputMessageMode = ui->inOutSelector->outputMessageMode());
  _filterThread->setInputImages(*_gmicImages,imageNames);
  connect(_filterThread,SIGNAL(finished()),
          this,SLOT(onApplyThreadFinished()));
  connect(_filterThread,SIGNAL(finished()),
          _filterThread,SLOT(deleteLater()));
  _waitingCursorTimer.start(WAITING_CURSOR_DELAY);
  ui->progressInfoWidget->startAnimationAndShow(_filterThread,true);
  _filterThread->start();
}

void
MainWindow::onApplyThreadFinished()
{
  ui->progressInfoWidget->stopAnimationAndHide();
  QStringList list = GmicStdLibParser::parseStatus(_filterThread->gmicStatus());
  if ( ! list.isEmpty() ) {
    ui->filterParams->setValues(list,false);
  }

  if ( QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::WaitCursor ) {
    QApplication::restoreOverrideCursor();
  }
  _waitingCursorTimer.stop();

  if ( _filterThread->failed() ) {
    _lastAppliedCommand.clear();
    _lastFilterName.clear();
    _lastAppliedCommandArguments.clear();
    _lastAppliedCommandOutputMessageMode = GmicQt::Quiet;
    QMessageBox::warning(this,tr("Error"),_filterThread->errorMessage(),QMessageBox::Close);
  } else {
    gmic_list<gmic_pixel_type> images = _filterThread->images();
    if ( ( _processingAction == OkAction || _processingAction == ApplyAction ) && !_filterThread->aborted() ) {
      gmic_qt_output_images(images,
                            _filterThread->imageNames(),
                            ui->inOutSelector->outputMode(),
                            (ui->inOutSelector->outputMessageMode() == GmicQt::VerboseLayerName) ?
                              QString("[G'MIC] %1: %2")
                              .arg(_filterThread->name())
                              .arg(_filterThread->fullCommand())
                              .toLatin1().constData()
                            : 0);
    }
  }
  _filterThread = 0;
  if ( ( _processingAction == OkAction || _processingAction == CloseAction ) ) {
    close();
  } else {
    LayersExtentProxy::clearCache();
    QSize extent = LayersExtentProxy::getExtent(ui->inOutSelector->inputMode());
    ui->previewWidget->setFullImageSize(extent);
    ui->previewWidget->sendUpdateRequest();
    _okButtonShouldApply = false;
  }
}

void
MainWindow::showWaitingCursor()
{
  if ( _filterThread && !(QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::WaitCursor) ) {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  }
}

void MainWindow::expandOrCollapseFolders()
{
  if ( _expandCollapseIcon == & _expandIcon) {
    ui->filtersTree->expandAll();
    ui->tbExpandCollapse->setIcon(_collapseIcon);
    _expandCollapseIcon = &_collapseIcon;
  } else {
    ui->tbExpandCollapse->setIcon(_expandIcon);
    ui->filtersTree->collapseAll();
    _expandCollapseIcon = &_expandIcon;
  }
}

void MainWindow::search(QString text)
{
  if ( text.length() < MINIMAL_SEARCH_LENGTH ) {
    ui->filtersTree->setModel(_currentFiltersTreeModel = &_filtersTreeModel);
    _filtersTreeModelSelection.clear();
    return;
  }
  _filtersTreeModelSelection.clear();
  QStandardItem * root = _filtersTreeModel.invisibleRootItem();
  Q_ASSERT(root);
  int rows = root->rowCount();
  for (int row = 0; row < rows; ++row) {
    FiltersTreeAbstractItem * item = dynamic_cast<FiltersTreeAbstractItem*>(root->child(row));
    Q_ASSERT(item);
    FiltersTreeAbstractItem * clone = item->deepClone(text.split(QChar(' '),QString::SkipEmptyParts));
    if ( clone ) {
      _filtersTreeModelSelection.appendRow(clone);
    }
  }
  _currentFiltersTreeModel = &_filtersTreeModelSelection;
  int count = FiltersTreeAbstractItem::countLeaves(_currentFiltersTreeModel->invisibleRootItem());
  _currentFiltersTreeModel->setHorizontalHeaderItem(0,new QStandardItem(QString(tr("Available filters (%1)")).arg(count)));
  ui->filtersTree->setModel(_currentFiltersTreeModel);
  ui->filtersTree->expandAll();
}

void
MainWindow::onApplyClicked()
{
  _processingAction = ApplyAction;
  processImage();
}

void
MainWindow::onOkClicked()
{
  if ( !selectedFilterItem() || ui->filterParams->command().isEmpty() || ui->filterParams->command() == "_none_" ) {
    close();
  }
  if ( _okButtonShouldApply ) {
    _processingAction = OkAction;
    processImage();
  } else {
    close();
  }
}

void
MainWindow::onCloseClicked()
{
  if ( _filterThread && confirmAbortProcessingOnCloseRequest() ) {
    _processingAction = CloseAction;
    if (_filterThread) {
      _filterThread->abortGmic();
    }
  } else {
    close();
  }
}

void
MainWindow::onCancelProcess()
{
  if ( _filterThread && _filterThread->isRunning() ) {
    _processingAction = NoAction;
    _filterThread->abortGmic();
  }
}

void
MainWindow::onReset()
{
  FiltersTreeAbstractFilterItem * filterItem = selectedFilterItem();
  FiltersTreeFaveItem * faveItem = filterItem ? dynamic_cast<FiltersTreeFaveItem*>(filterItem) : 0;
  if ( faveItem ) {
    ui->filterParams->setValues(faveItem->defaultValues(),true);
    return;
  }
  if ( ! ui->filterParams->filterName().isEmpty() ) {
    ui->filterParams->reset(true);
  }
}

void MainWindow::onPreviewZoomReset()
{
  FiltersTreeAbstractFilterItem * filterItem = selectedFilterItem();
  if ( filterItem ) {
    ui->previewWidget->setPreviewFactor(filterItem->previewFactor(),true);
    ui->previewWidget->sendUpdateRequest();
    ui->labelWarning->setPixmap(QPixmap(":/images/no_warning.png"));
  }
}

void MainWindow::onUpdateFiltersClicked()
{
  ui->tbUpdateFilters->setEnabled(false);
  updateFiltersFromSources(0, ui->cbInternetUpdate->isChecked());
}

void MainWindow::saveCurrentParameters()
{
  if ( ! ui->filterParams->pathHash().isEmpty() ) {
    QString hash = ui->filterParams->pathHash();
    ParametersCache::setValue(hash, ui->filterParams->valueStringList());
    ParametersCache::setInputOutputState(hash, ui->inOutSelector->state());
  }
}

void
MainWindow::saveSettings()
{
  DialogSettings::saveSettings();
  QSettings settings;
  ui->inOutSelector->save(settings);
  QString selectedFilterHash;
  FiltersTreeAbstractFilterItem * filterItem = selectedFilterItem();
  if ( filterItem ) {
    selectedFilterHash = filterItem->hash();
  }
  settings.setValue("LastExecution/gmic_version",gmic_version);
  settings.setValue(QString("LastExecution/host_%1/Command").arg(GmicQt::HostApplicationShortname),_lastAppliedCommand);
  settings.setValue(QString("LastExecution/host_%1/FilterName").arg(GmicQt::HostApplicationShortname),_lastFilterName);
  settings.setValue(QString("LastExecution/host_%1/Arguments").arg(GmicQt::HostApplicationShortname),_lastAppliedCommandArguments);
  settings.setValue(QString("LastExecution/host_%1/OutputMessageMode").arg(GmicQt::HostApplicationShortname),_lastAppliedCommandOutputMessageMode);
  settings.setValue(QString("LastExecution/host_%1/InputMode").arg(GmicQt::HostApplicationShortname),ui->inOutSelector->inputMode());
  settings.setValue(QString("LastExecution/host_%1/OutputMode").arg(GmicQt::HostApplicationShortname),ui->inOutSelector->outputMode());
  settings.setValue(QString("LastExecution/host_%1/PreviewMode").arg(GmicQt::HostApplicationShortname),ui->inOutSelector->previewMode());
  settings.setValue(QString("LastExecution/host_%1/GmicEnvironment").arg(GmicQt::HostApplicationShortname),ui->inOutSelector->gmicEnvString());
  settings.setValue("SelectedFilter",selectedFilterHash);
  settings.setValue("Config/MainWindowPosition",frameGeometry().topLeft());
  settings.setValue("Config/MainWindowRect",rect());
  settings.setValue("Config/MainWindowMaximized",isMaximized());
  settings.setValue("Config/ShowAllFilters",filtersSelectionMode());
  settings.setValue("LastExecution/ExitedNormally",true);
  settings.setValue("LastExecution/HostApplicationID",GmicQt::host_app_pid());
  QList<int> spliterSizes = ui->splitter->sizes();
  for ( int i = 0; i < spliterSizes.size(); ++i )  {
    settings.setValue(QString("Config/PanelSize%1").arg(i),spliterSizes.at(i));
  }
  settings.setValue(REFRESH_USING_INTERNET_KEY,ui->cbInternetUpdate->isChecked());

  backupExpandedFoldersPaths();
  settings.setValue("Config/ExpandedFolders",_expandedFoldersPaths);
}

void
MainWindow::loadSettings()
{
  DialogSettings::loadSettings();
  FiltersVisibilityMap::load();
  QSettings settings;
  _lastExecutionOK = settings.value("LastExecution/ExitedNormally",true).toBool();
  _newSession = GmicQt::host_app_pid() != settings.value("LastExecution/HostApplicationID",0).toUInt();
  settings.setValue("LastExecution/ExitedNormally",false);
  if ( _newSession ) {
    ui->inOutSelector->reset();
  } else {
    ui->inOutSelector->load(settings);
  }
  // Preview position
  if ( settings.value("Config/PreviewPosition","Left").toString() == "Left" ) {
    setPreviewPosition(PreviewOnLeft);
  }
  if ( DialogSettings::darkThemeEnabled() ) {
    setDarkTheme();
  }

  // Mainwindow geometry
  QPoint position = settings.value("Config/MainWindowPosition",QPoint()).toPoint();
  QRect r = settings.value("Config/MainWindowRect",QRect()).toRect();
  _showMaximized = settings.value("Config/MainWindowMaximized",false).toBool();
  if ( ! _showMaximized  ) {
    if ( r.isValid() ) {
      setGeometry(r);
      move(position);
    } else {
      QDesktopWidget desktop;
      QRect screenSize = desktop.availableGeometry();
      screenSize.setWidth(screenSize.width()*0.66);
      screenSize.setHeight(screenSize.height()*0.66);
      screenSize.moveCenter(desktop.availableGeometry().center());
      setGeometry(screenSize);
      int w = screenSize.width();
      ui->splitter->setSizes(QList<int>() << (w*0.4) << (w*0.2) << (w*0.4));
    }
  }
  // Splitter sizes
  QList<int> sizes;
  for ( int i = 0; i < 3; ++i )  {
    int s = settings.value(QString("Config/PanelSize%1").arg(i),0).toInt();
    if ( s ) {
      sizes.push_back(s);
    }
  }
  if ( sizes.size() == 3 ) {
    ui->splitter->setSizes(sizes);
  }

  // Filters visibility
  ui->cbFiltersSelectionMode->setChecked(settings.value("Config/ShowAllFilters",false).toBool());
  ui->cbInternetUpdate->setChecked(settings.value("Config/RefreshInternetUpdate",true).toBool());

  // This will not be overwritten by the first call of backupExpandedFoldersPaths()
  _expandedFoldersPaths = settings.value("Config/ExpandedFolders",QStringList()).toStringList();
}

void MainWindow::setPreviewPosition(MainWindow::PreviewPosition position)
{
  if ( position == _previewPosition ) {
    return;
  }
  _previewPosition = position;

  QHBoxLayout * layout = dynamic_cast<QHBoxLayout*>( ui->belowPreviewWidget->layout() );
  if ( layout ) {
    layout->removeWidget(ui->inOutSelector);
    layout->removeWidget(ui->belowPreviewPadding);
    layout->removeWidget(ui->logosLabel);
    if ( position == MainWindow::PreviewOnLeft ) {
      layout->addWidget(ui->logosLabel);
      layout->addWidget(ui->belowPreviewPadding);
      layout->addWidget(ui->inOutSelector);
    } else {
      layout->addWidget(ui->inOutSelector);
      layout->addWidget(ui->belowPreviewPadding);
      layout->addWidget(ui->logosLabel);
    }
  }

  // Swap splitter widgets
  QWidget * preview;
  QWidget * list;
  QWidget * params;
  if ( position == MainWindow::PreviewOnRight ) {
    ui->messageLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    preview  = ui->splitter->widget(0);
    list = ui->splitter->widget(1);
    params = ui->splitter->widget(2);
  } else {
    ui->messageLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    list = ui->splitter->widget(0);
    params = ui->splitter->widget(1);
    preview  = ui->splitter->widget(2);
  }
  preview->hide();
  list->hide();
  params->hide();
  preview->setParent(this);
  list->setParent(this);
  params->setParent(this);
  if ( position == MainWindow::PreviewOnRight ) {
    ui->splitter->addWidget(list);
    ui->splitter->addWidget(params);
    ui->splitter->addWidget(preview);
  } else {
    ui->splitter->addWidget(preview);
    ui->splitter->addWidget(list);
    ui->splitter->addWidget(params);
  }
  preview->show();
  list->show();
  params->show();

  ui->logosLabel->setAlignment(Qt::AlignVCenter | ((_previewPosition == PreviewOnRight)?Qt::AlignRight:Qt::AlignLeft) );
}

QImage MainWindow::buildPreviewImage(const cimg_library::CImgList<float> & images)
{
  QImage qimage;
  cimg_library::CImgList<gmic_pixel_type> preview_input_images;
  switch (ui->inOutSelector->previewMode()) {
  case GmicQt::FirstOutput:
    if (images && images.size()>0) {
      preview_input_images.push_back(images[0]);
    }
    break;
  case GmicQt::SecondOutput:
    if (images && images.size()>1) {
      preview_input_images.push_back(images[1]);
    }
    break;
  case GmicQt::ThirdOutput:
    if (images && images.size()>2) {
      preview_input_images.push_back(images[2]);
    }
    break;
  case GmicQt::FourthOutput:
    if (images && images.size()>3) {
      preview_input_images.push_back(images[3]);
    }
    break;
  case GmicQt::First2SecondOutput:
  {
    preview_input_images.push_back(images[0]);
    preview_input_images.push_back(images[1]);
  }
    break;
  case GmicQt::First2ThirdOutput:
  {
    for (int i = 0; i < 3; ++i) {
      preview_input_images.push_back(images[i]);
    }
  }
    break;
  case GmicQt::First2FourthOutput:
  {
    for (int i = 0; i < 4; ++i) {
      preview_input_images.push_back(images[i]);
    }
  }
    break;
  case GmicQt::AllOutputs:
  default:
    preview_input_images = images;
  }

  int spectrum = 0;
  cimglist_for(preview_input_images,l) {
    spectrum = std::max(spectrum,preview_input_images[l].spectrum());
  }
  spectrum += ( spectrum == 1 || spectrum == 3);
  cimglist_for(preview_input_images,l) {
    GmicQt::calibrate_image(preview_input_images[l],spectrum,true);
  }
  if (preview_input_images.size() == 1) {
    ImageConverter::convert(preview_input_images.front(),qimage);
    return qimage;
  }
  if (preview_input_images.size()  > 1) {
    try {
      cimg_library::CImgList<char> preview_images_names;
      gmic("-v - -gui_preview",
           preview_input_images,
           preview_images_names,
           GmicStdLibParser::GmicStdlib.constData(),
           true);
      if (preview_input_images.size() >= 1) {
        ImageConverter::convert(preview_input_images.front(),qimage);
        return qimage;
      }
    } catch (...) {
      qimage = QImage(ui->previewWidget->size(),QImage::Format_ARGB32);
      QPainter painter(&qimage);
      painter.fillRect(qimage.rect(),QColor(40,40,40,200));
      painter.setPen(Qt::green);
      painter.drawText(qimage.rect(),
                       Qt::AlignCenter|Qt::TextWordWrap,
                       "Preview error (handling preview mode)");
      painter.end();
      return qimage;
    }
  }
  qimage = QImage(10,10,QImage::Format_ARGB32);
  qimage.fill(Qt::black);
  return qimage;
}

void MainWindow::onFiltersTreeItemChanged(QStandardItem * item)
{
  if ( ! item->isCheckable() ) {
    return;
  }
  int row = item->index().row();
  QStandardItem * parentFolder = item->parent();
  if ( !parentFolder ) {
    // parent is 0 for top level items
    parentFolder = _filtersTreeModel.invisibleRootItem();
  }
  QStandardItem * leftItem = parentFolder->child(row);
  FiltersTreeFolderItem * folder = dynamic_cast<FiltersTreeFolderItem*>(leftItem);
  if ( folder ) {
    folder->applyVisibilityStatusToFolderContents();
  }
  // Force an update of the view by triggering a call of QStandardItem::emitDataChanged()
  leftItem->setData(leftItem->data());
}

bool MainWindow::filtersSelectionMode()
{
  return ui->cbFiltersSelectionMode->isChecked();
}

QString
MainWindow::faveUniqueName(const QString & name)
{
  FiltersTreeFolderItem * folder = faveFolder(FullModel);
  if ( ! folder ) {
    return name;
  }
  int count = folder->rowCount();
  int n = 1;
  for (int i = 0; i < count; ++i) {
    QString s = folder->child(i)->text();
    s.replace(QRegExp(" \\(\\d*\\)"),"");
    if (s == name) {
      ++n;
    }
  }
  if (n > 1) {
    return QString("%1 (%2)").arg(name).arg(n);
  } else {
    return name;
  }
}

void
MainWindow::selectFilter(QModelIndex index, bool resetZoom, const QList<QString> & values)
{
  FiltersTreeAbstractFilterItem * filterItem;
  filterItem = dynamic_cast<FiltersTreeAbstractFilterItem*>( _currentFiltersTreeModel->itemFromIndex(index) );
  FiltersTreeFaveItem * faveItem = filterItem ? dynamic_cast<FiltersTreeFaveItem*>(filterItem) : 0;
  saveCurrentParameters();

  // Restore fave's Input/Ouput panel settings, or restore previous ones
  ui->inOutSelector->disableNotifications();
  if ( faveItem ) {
    ui->inOutSelector->saveState();
    faveItem->getInOutSettings(ui->inOutSelector);
  } else {
    ui->inOutSelector->restoreState();
  }
  ui->inOutSelector->enableNotifications();

  if ( filterItem ) {
    ui->filterParams->build(filterItem,values);
    ui->filterName->setText(QString("<b>%1</b>").arg(filterItem->text()));
    ui->filterName->setVisible(true);
    ui->tbAddFave->setEnabled(true);
    ui->previewWidget->setPreviewFactor(filterItem->previewFactor(),resetZoom);
    showZoomWarningIfNeeded();
    _okButtonShouldApply = true;
    ui->tbResetParameters->setVisible(true);
  } else {
    ui->filterParams->setNoFilter();
    ui->filterName->setVisible(false);
    ui->tbAddFave->setEnabled(false);
    ui->tbResetParameters->setVisible(false);
    ui->labelWarning->setPixmap(QPixmap(":/images/no_warning.png"));
    _okButtonShouldApply = false;
  }
  ui->tbRemoveFave->setEnabled(faveItem != nullptr);
  ui->tbRenameFave->setEnabled(faveItem != nullptr);
}

FiltersTreeAbstractFilterItem *
MainWindow::selectedFilterItem()
{
  // Get filter item even if checkbox is selected

  QModelIndex index = ui->filtersTree->currentIndex();
  QStandardItem * item = _currentFiltersTreeModel->itemFromIndex(index);
  if ( item ) {
    int row = index.row();
    QStandardItem * parentFolder = item->parent();
    // parent is 0 for top level items
    if ( !parentFolder ) {
      parentFolder = _currentFiltersTreeModel->invisibleRootItem();
    }
    QStandardItem * leftItem = parentFolder->child(row,0);
    if ( leftItem ) {
      FiltersTreeAbstractFilterItem * filter = dynamic_cast<FiltersTreeAbstractFilterItem*>(leftItem);
      if ( filter ) {
        return filter;
      }
    }
  }
  return 0;
}

void MainWindow::showEvent(QShowEvent * event)
{
  static bool first = true;
  event->accept();
  ui->searchField->setFocus();
  if ( first ) {
    first = false;
    importFaves();
    buildFiltersTree();

    // Retrieve and select previously selected filter
    QString hash = QSettings().value("SelectedFilter",QString()).toString();
    if (_newSession || !_lastExecutionOK) {
      hash.clear();
    }
    if ( !hash.isEmpty() ) {
      FiltersTreeAbstractFilterItem * filterItem = findFilter(hash,FullModel);
      if ( !filterItem ) {
        filterItem = findFave(hash,FullModel);
      }
      if ( filterItem ) {
        ui->filtersTree->setCurrentIndex(filterItem->index());
        ui->filtersTree->scrollTo(filterItem->index(),QAbstractItemView::PositionAtCenter);
        selectFilter(filterItem->index(),true);
        // Preview update is triggered when PreviewWidget receives
        // the WindowActivate Event (while pendingResize is true
        // after the very first resize event).
        return;
      }
    } else {
      // Expand fave folder
      FiltersTreeFolderItem * faves = faveFolder(FullModel);
      if ( faves ) {
        QModelIndex index = faves->index();
        ui->filtersTree->expand(index);
      }
      ui->previewWidget->setPreviewFactor(GmicQt::PreviewFactorFullImage,true);
    }
    ui->filtersTree->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    ui->filtersTree->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    ui->filtersTree->adjustSize();
  }
}

void MainWindow::resizeEvent(QResizeEvent * e)
{
  // Check if size is reducing
  if ( (e->size().width() < e->oldSize().width() ||
        e->size().height() < e->oldSize().height() )
       && ui->pbFullscreen->isChecked()
       && (windowState() & Qt::WindowMaximized) ) {
    ui->pbFullscreen->toggle();
  }
}

FiltersTreeFolderItem  *
MainWindow::faveFolder(ModelType modelType)
{
  QStandardItemModel & model = modelType == FullModel ? _filtersTreeModel : _filtersTreeModelSelection;
  QStandardItem * root = model.invisibleRootItem();
  int count = root->rowCount();
  for (int i = 0; i < count; ++i) {
    FiltersTreeFolderItem * item = dynamic_cast<FiltersTreeFolderItem*>( root->child(i) );
    if ( item && item->isFaveFolder() ) {
      return item;
    }
  }
  return 0;
}

FiltersTreeFaveItem * MainWindow::findFave(const QString & hash, ModelType modelType)
{
  FiltersTreeFolderItem * folder = faveFolder(modelType);
  return folder ? FiltersTreeAbstractItem::findFave(folder,hash) : 0;
}

FiltersTreeFilterItem *
MainWindow::findFilter(const QString &hash, MainWindow::ModelType modelType)
{
  QStandardItemModel & model = modelType == FullModel ? _filtersTreeModel : _filtersTreeModelSelection;
  FiltersTreeFilterItem * filter = FiltersTreeAbstractItem::findFilter(model.invisibleRootItem(),hash);
  return filter;
}

void
MainWindow::addFaveFolder()
{
  if ( ! faveFolder(FullModel) ) {
    FiltersTreeFolderItem * faveFolder = new FiltersTreeFolderItem(tr(FAVE_FOLDER_TEXT),
                                                                   FiltersTreeFolderItem::FaveFolder);
    if ( filtersSelectionMode() ) {
      GmicStdLibParser::addStandardItemWithCheckBox(_filtersTreeModel.invisibleRootItem(),faveFolder,true);
    } else {
      _filtersTreeModel.invisibleRootItem()->appendRow(faveFolder);
    }
    _filtersTreeModel.invisibleRootItem()->sortChildren(0);
  }
}

void
MainWindow::removeFaveFolder()
{
  QStandardItem * rootFull = _filtersTreeModel.invisibleRootItem();
  FiltersTreeFolderItem * faveFolderFull = faveFolder(FullModel);
  for (int row = 0; row < rootFull->rowCount() && faveFolderFull ; ++row ) {
    QStandardItem * item = rootFull->child(row);
    if (item == faveFolderFull) {
      _filtersTreeModel.removeRow(row,item->index().parent());
      faveFolderFull = 0;
    }
  }


  QStandardItem * rootSelection = _filtersTreeModel.invisibleRootItem();
  FiltersTreeFolderItem * faveFolderSelection = faveFolder(SelectionModel);
  for (int row = 0; row < rootSelection->rowCount() && faveFolderSelection; ++row ) {
    QStandardItem * item = rootSelection->child(row);
    if (item == faveFolderSelection) {
      _filtersTreeModel.removeRow(row,item->index().parent());
      faveFolderSelection = 0;
    }
  }
}

void
MainWindow::loadFaves(bool withVisibility)
{
  Q_ASSERT( ui->filtersTree->model() );
  if ( withVisibility ) {
    qDeleteAll(_hiddenFaves);
    _hiddenFaves.clear();
  }
  bool imported = ! _importedFaves.isEmpty();
  QList<StoredFave> faves = StoredFave::readFaves();
  if ( faves.isEmpty() && _importedFaves.isEmpty() ) {
    return;
  }
  addFaveFolder();
  FiltersTreeFolderItem * folder = faveFolder(FullModel);
  faves.append(_importedFaves);
  for ( StoredFave & importedFave : faves ) {
    QString hash = importedFave.originalFilterHash();
    FiltersTreeFilterItem * filterItem = findFilter(hash,FullModel);
    if ( filterItem ) {
      FiltersTreeFaveItem * faveItem = new FiltersTreeFaveItem(filterItem,
                                                               importedFave.name(),
                                                               importedFave.defaultParameters());
      faveItem->setInOutSettings(importedFave.inputMode(),
                                 importedFave.outputMode(),
                                 importedFave.previewMode(),
                                 importedFave.outputMessageMode());
      bool faveIsVisible = FiltersVisibilityMap::filterIsVisible(faveItem->hash());
      if ( withVisibility ) {
        GmicStdLibParser::addStandardItemWithCheckBox(folder,faveItem,faveIsVisible);
      } else {
        if ( faveIsVisible ) {
          folder->appendRow(faveItem);
        } else {
          _hiddenFaves.push_back(faveItem);
        }
      }
    }
  }
  faveFolder(FullModel)->sortChildren(0);
  if ( imported ) {
    saveFaves();
    QSettings().setValue(FAVES_IMPORT_KEY,true);
  }
}

bool
MainWindow::importFaves()
{
  QFileInfo info(QString("%1%2").arg(GmicQt::path_rc(false)).arg("gimp_faves"));
  if ( !info.isReadable() || QSettings().value(FAVES_IMPORT_KEY,false).toBool() ) {
    return false;
  }

  QMessageBox messageBox(QMessageBox::Question,
                         tr("Import faves"),
                         QString(tr("Do you want to import faves from file below?<br/>%1")).arg(info.filePath()),
                         QMessageBox::Yes | QMessageBox::No,
                         this);
  messageBox.setDefaultButton(QMessageBox::Yes);
  QCheckBox * cb = new QCheckBox(tr("Don't ask again"));
  if ( DialogSettings::darkThemeEnabled() ) {
    QPalette p = cb->palette();
    p.setColor(QPalette::Text, DialogSettings::CheckBoxTextColor);
    p.setColor(QPalette::Base, DialogSettings::CheckBoxBaseColor);
    cb->setPalette(p);
  }
  messageBox.setCheckBox(cb);
  int choice = messageBox.exec();
  if (choice != QMessageBox::Yes) {
    if (cb->isChecked()) {
      QSettings().setValue(FAVES_IMPORT_KEY,true);
    }
    return false;
  }

  _importedFaves = StoredFave::importFaves();
  return ! _importedFaves.isEmpty();
}

void
MainWindow::saveFaves()
{
  FiltersTreeFolderItem * folder = faveFolder(FullModel);
  QString filename(QString("%1%2").arg(GmicQt::path_rc(true)).arg("gmic_qt_faves"));
  QString jsonFilename(QString("%1%2").arg(GmicQt::path_rc(true)).arg("gmic_qt_faves.json"));
  if ( folder || _hiddenFaves.size() ) {
    QFile::remove(filename+".bak");
    QFile::copy(filename,filename+".bak");
    QFile file(filename);
    if (file.open(QFile::WriteOnly|QFile::Truncate)) {
      QTextStream stream(&file);
      stream << QString("@gmic_qt_faves Version=%1.%2.%3\n").arg(gmic_version/100).arg((gmic_version/10)%10).arg(gmic_version%10).toLocal8Bit();
      stream << "#\n"
                "# You can copy this file to save or move your faves.\n"
                "# If you happen to modify it, be careful!\n"
                "#\n";
      if ( folder ) {
        int count = folder->rowCount();
        for (int row = 0; row < count; ++row) {
          FiltersTreeFaveItem * fave = static_cast<FiltersTreeFaveItem*>(folder->child(row));
          stream << StoredFave(fave);
        }
      }
      for ( FiltersTreeFaveItem * fave : _hiddenFaves ) {
        stream << StoredFave(fave);
      }
    } else {
      std::cerr << "[gmic_qt] Error: cannot open/create file " << filename.toStdString() << std::endl;
    }

    // Create JSON array
    QJsonArray array;
    if ( folder ) {
      int count = folder->rowCount();
      for (int row = 0; row < count; ++row) {
        FiltersTreeFaveItem * fave = static_cast<FiltersTreeFaveItem*>(folder->child(row));
        array.append(StoredFave(fave).toJSONObject());
      }
    }
    for ( FiltersTreeFaveItem * fave : _hiddenFaves ) {
      array.append(StoredFave(fave).toJSONObject());
    }
    // Save JSON array
    QFile jsonFile(jsonFilename);
    if ( jsonFile.open(QIODevice::WriteOnly|QIODevice::Truncate) ) {
      QJsonDocument jsonDoc(array);
      jsonFile.write(jsonDoc.toJson());
    } else {
      std::cerr << "[gmic_qt] Error: cannot open/create file " << jsonFilename.toStdString() << std::endl;
    }

  } else {
    // Backup current file
    QFile::copy(filename,filename+".bak");
    QFile::remove(filename);
  }
}

void
MainWindow::onAddFave()
{
  QModelIndex index = ui->filtersTree->currentIndex();
  FiltersTreeAbstractFilterItem * item = dynamic_cast<FiltersTreeAbstractFilterItem*>( _currentFiltersTreeModel->itemFromIndex(index) );
  if ( item ) {
    if ( ! faveFolder(FullModel) ) {
      addFaveFolder();
    }
    FiltersTreeFaveItem * fave = new FiltersTreeFaveItem(item,
                                                         faveUniqueName(item->text()),
                                                         ui->filterParams->valueStringList());
    fave->setInOutSettings(ui->inOutSelector);
    FiltersTreeFolderItem  * folder = faveFolder(FullModel);

    if ( filtersSelectionMode() ) {
      GmicStdLibParser::addStandardItemWithCheckBox(folder,fave,true);
    } else {
      folder->appendRow(fave);
    }

    ParametersCache::setValue(fave->hash(),ui->filterParams->valueStringList());
    ParametersCache::setInputOutputState(fave->hash(), ui->inOutSelector->state());
    ui->filtersTree->setCurrentIndex(fave->index());
    folder->sortChildren(0,Qt::AscendingOrder);
    saveFaves();
    ui->tbRemoveFave->setEnabled(true);
    ui->tbRenameFave->setEnabled(true);
  }
}

void
MainWindow::onRemoveFave()
{
  FiltersTreeAbstractFilterItem * item = selectedFilterItem();
  FiltersTreeFaveItem * fave = item ? dynamic_cast<FiltersTreeFaveItem *>(item) : 0;
  if ( fave ) {
    ParametersCache::remove(fave->hash());
    QString hash = fave->hash();
    FiltersTreeFaveItem * item = findFave(hash,FullModel);
    _filtersTreeModel.removeRow(item->row(),item->index().parent());
    item = findFave(hash,SelectionModel);
    if ( item ) {
      _filtersTreeModelSelection.removeRow(item->row(),item->index().parent());
    }
    saveFaves();
  }
  if ( faveFolder(FullModel)->rowCount() == 0) {
    removeFaveFolder();
    ui->tbRemoveFave->setEnabled(false);
    ui->tbRenameFave->setEnabled(false);
  }
}

void
MainWindow::onRenameFave()
{
  FiltersTreeAbstractFilterItem * item = selectedFilterItem();
  FiltersTreeFaveItem * fave = item ? dynamic_cast<FiltersTreeFaveItem *>(item) : 0;
  if ( fave ) {
    ui->filtersTree->edit(fave->index());
    //    QString newName = QInputDialog::getText(this,
    //                                            tr("Rename a fave"),
    //                                            tr("Enter a new name"),
    //                                            QLineEdit::Normal,
    //                                            item->text(),
    //                                            0);
    //    if ( newName.isNull() ) {
    //      return;
    //    }
    //    QString hash = item->hash();
    //    ParametersCache::remove(hash);
    //    FiltersTreeFaveItem * item = findFave(hash,FullModel);
    //    item->rename(newName);
    //    ParametersCache::setValue(item->hash(),ui->filterParams->valueStringList());
    //    if ( ( item = findFave(hash,SelectionModel) ) ) {
    //      item->rename(newName);
    //    }
    //    FiltersTreeFolderItem * folder = faveFolder(FullModel);
    //    if ( folder ) {
    //      folder->sortChildren(0);
    //    }
    //    folder = faveFolder(SelectionModel);
    //    if ( folder ) {
    //      folder->sortChildren(0);
    //    }
  }
}

void MainWindow::onRenameFaveFinished(QWidget * editor)
{
  QLineEdit * le = dynamic_cast<QLineEdit*>(editor);
  Q_ASSERT_X(le,"Rename Fave","Editor is not a QLineEdit!");
  FiltersTreeAbstractFilterItem * item = selectedFilterItem();
  FiltersTreeFaveItem * fave = item ? dynamic_cast<FiltersTreeFaveItem *>(item) : 0;
  if ( !fave ) {
    return;
  }
  QString newName = le->text();
  if ( newName.isEmpty() ) {
    FiltersTreeFilterItem * filter = findFilter(fave->originalFilterHash(),FullModel);
    if ( filter ) {
      newName = faveUniqueName(filter->name());
    } else {
      newName = faveUniqueName("Unkown filter");
    }
  }

  QString hash = fave->hash();
  QList<QString> values = ParametersCache::getValue(hash);
  InOutPanel::State inOutState = ParametersCache::getInputOutputState(hash);
  ParametersCache::remove(hash);
  fave->rename(newName);
  ParametersCache::setValue(fave->hash(),values);
  ParametersCache::setInputOutputState(fave->hash(),inOutState);

  FiltersTreeFaveItem * selectedFave = findFave(hash,SelectionModel);
  if ( selectedFave ) {
    selectedFave->rename(newName);
  }
  FiltersTreeFolderItem * folder = faveFolder(FullModel);
  if ( folder ) {
    folder->sortChildren(0);
  }
  folder = faveFolder(SelectionModel);
  if ( folder ) {
    folder->sortChildren(0);
  }
  saveFaves();
}

void
MainWindow::onOutputMessageModeChanged(GmicQt::OutputMessageMode mode)
{
  if ( mode == GmicQt::VerboseLogFile || mode == GmicQt::VeryVerboseLogFile || mode == GmicQt::DebugLogFile ) {
    QString filename = QString("%1gmic_qt_log").arg(GmicQt::path_rc(true));
    _logFile = fopen(filename.toLocal8Bit().constData(),"a");
    cimg_library::cimg::output(_logFile ? _logFile : stdout);
  } else {
    if ( _logFile ) {
      std::fclose(_logFile);
      _logFile = 0;
    }
    cimg_library::cimg::output(stdout);
  }
}

void MainWindow::onToggleFullScreen(bool on)
{
  if ( on && !(windowState() & Qt::WindowMaximized) ) {
    showMaximized();
  }
  if ( !on && (windowState() & Qt::WindowMaximized) ) {
    showNormal();
  }
}

void MainWindow::onSettingsClicked()
{
  QList<int> splitterSizes = ui->splitter->sizes();

  int previewWidth;
  int paramsWidth;
  int treeWidth;
  if ( _previewPosition == PreviewOnLeft ) {
    previewWidth = splitterSizes.at(0);
    paramsWidth = splitterSizes.at(2);
    treeWidth = splitterSizes.at(1);
  } else {
    previewWidth = splitterSizes.at(2);
    paramsWidth = splitterSizes.at(1);
    treeWidth = splitterSizes.at(0);
  }

  DialogSettings dialog(this);
  dialog.exec();
  bool previewPositionChanged = (_previewPosition != DialogSettings::previewPosition());
  setPreviewPosition(dialog.previewPosition());
  if ( previewPositionChanged ) {
    splitterSizes.clear();
    if ( _previewPosition == PreviewOnLeft ) {
      splitterSizes.push_back(previewWidth);
      splitterSizes.push_back(treeWidth);
      splitterSizes.push_back(paramsWidth);
    } else {
      splitterSizes.push_back(treeWidth);
      splitterSizes.push_back(paramsWidth);
      splitterSizes.push_back(previewWidth);
    }
    ui->splitter->setSizes(splitterSizes);
  }
}

bool MainWindow::confirmAbortProcessingOnCloseRequest()
{
  int button = QMessageBox::question(this,
                                     tr("Confirmation"),
                                     tr("A gmic command is running.<br>Do you really want to close the plugin?"),
                                     QMessageBox::Yes,
                                     QMessageBox::No);
  return ( button == QMessageBox::Yes );
}

void MainWindow::closeEvent(QCloseEvent * e)
{
  if ( _filterThread ) {
    if ( confirmAbortProcessingOnCloseRequest() ) {
      _filterThread->abortGmic();
      _processingAction = CloseAction;
    }
    e->ignore();
  } else {
    e->accept();
  }
}

void MainWindow::backupExpandedFoldersPaths()
{
  // Do nothing if a search result is displayed
  // or if the filters tree is empty
  if ( (_currentFiltersTreeModel == &_filtersTreeModelSelection) ||
       ( _filtersTreeModel.invisibleRootItem()->rowCount() == 0) ) {
    return;
  }
  _expandedFoldersPaths.clear();
  expandedFolderPaths(_filtersTreeModel.invisibleRootItem(),_expandedFoldersPaths);
}

void MainWindow::expandedFolderPaths(QStandardItem * item, QStringList & list)
{
  int rows = item->rowCount();
  for (int row = 0; row < rows; ++row) {
    FiltersTreeFolderItem * subFolder = dynamic_cast<FiltersTreeFolderItem*>(item->child(row));
    if ( subFolder ) {
      if (ui->filtersTree->isExpanded(subFolder->index())) {
        list.push_back(subFolder->path().join(FilterTreePathSeparator));
      }
      expandedFolderPaths(subFolder,list);
    }
  }
}

void MainWindow::restoreExpandedFolders()
{
  if ( _currentFiltersTreeModel == &_filtersTreeModelSelection ) {
    return;
  }
  restoreExpandedFolders(_filtersTreeModel.invisibleRootItem());
}

void MainWindow::restoreExpandedFolders(QStandardItem *item)
{
  int rows = item->rowCount();
  for (int row = 0; row < rows; ++row) {
    FiltersTreeFolderItem * subFolder = dynamic_cast<FiltersTreeFolderItem*>(item->child(row));
    if ( subFolder ) {
      if ( _expandedFoldersPaths.contains(subFolder->path().join(FilterTreePathSeparator)) ) {
        ui->filtersTree->expand(subFolder->index());
      } else {
        ui->filtersTree->collapse(subFolder->index());
      }
      restoreExpandedFolders(subFolder);
    }
  }
}

QString MainWindow::treeIndexToPath(const QModelIndex index)
{
  QStandardItem * item = _filtersTreeModel.itemFromIndex(index);
  if ( item ) {
    FiltersTreeAbstractItem * filter = dynamic_cast<FiltersTreeAbstractItem*>(item);
    if ( filter ) {
      return filter->path().join(FilterTreePathSeparator);
    }
  }
  return QString();
}

QModelIndex MainWindow::treePathToIndex(const QString path)
{
  return treePathToIndex(path,_filtersTreeModel.invisibleRootItem());
}

QModelIndex MainWindow::treePathToIndex(const QString path, QStandardItem * item)
{
  // Search at current level first
  int rows = item->rowCount();
  for (int row = 0; row < rows; ++row) {
    FiltersTreeAbstractFilterItem * filter = dynamic_cast<FiltersTreeAbstractFilterItem*>(item->child(row));
    if ( filter ) {
      if ( filter->path().join(FilterTreePathSeparator) == path ) {
        return filter->index();
      }
    }
  }
  // Then do a depth-first search
  QModelIndex result;
  for (int row = 0; row < rows; ++row) {
    FiltersTreeFolderItem * folder = dynamic_cast<FiltersTreeFolderItem*>(item->child(row));
    if ( folder ) {
      result = treePathToIndex(path,folder);
      if ( result.isValid() ) {
        return result;
      }
    }
  }
  return QModelIndex();
}
