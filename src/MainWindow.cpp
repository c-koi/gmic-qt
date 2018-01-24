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
#include "MainWindow.h"
#include <DialogSettings.h>
#include <QAbstractItemModel>
#include <QAction>
#include <QCursor>
#include <QDebug>
#include <QDesktopWidget>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeySequence>
#include <QList>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>
#include <QShowEvent>
#include <QStyleFactory>
#include <QTextDocument>
#include <QTextStream>
#include <cassert>
#include <iostream>
#include <typeinfo>
#include "Common.h"
#include "FilterSelector/FavesModelReader.h"
#include "FilterSelector/FiltersPresenter.h"
#include "FilterSelector/FiltersVisibilityMap.h"
#include "FilterThread.h"
#include "GmicStdlib.h"
#include "Host/host.h"
#include "ImageConverter.h"
#include "ImageTools.h"
#include "LayersExtentProxy.h"
#include "Logger.h"
#include "ParametersCache.h"
#include "Updater.h"
#include "ui_mainwindow.h"
#include "gmic.h"

//
// TODO : Handle window maximization properly (Windows as well as some Linux desktops)
//

const QString MainWindow::FilterTreePathSeparator("\t");

MainWindow::MainWindow(QWidget * parent) : QWidget(parent), ui(new Ui::MainWindow), _gmicImages(new cimg_library::CImgList<gmic_pixel_type>), _filterThread(0)
{
  ui->setupUi(this);
  _messageTimerID = 0;
  _gtkFavesShouldBeImported = false;
  _lastAppliedCommandOutputMessageMode = GmicQt::UnspecifiedOutputMessageMode;
#ifdef gmic_prerelease
#define BETA_SUFFIX "_pre#" gmic_prerelease
#else
#define BETA_SUFFIX ""
#endif

  setWindowTitle(QString("G'MIC-Qt %1- %2 %3 bits - %4" BETA_SUFFIX)
                     .arg(GmicQt::HostApplicationName.isEmpty() ? QString() : QString("for %1 ").arg(GmicQt::HostApplicationName))
                     .arg(cimg_library::cimg::stros())
                     .arg(sizeof(void *) == 8 ? 64 : 32)
                     .arg(GmicQt::gmicVersionString()));

  QStringList tsp = QIcon::themeSearchPaths();
  tsp.append(QString("/usr/share/icons/gnome"));
  QIcon::setThemeSearchPaths(tsp);

  _filterUpdateWidgets = {ui->previewWidget,   ui->tbZoomIn,     ui->tbZoomOut,  ui->tbZoomReset, ui->filtersView, ui->filterParams,
                          ui->tbUpdateFilters, ui->pbFullscreen, ui->pbSettings, ui->pbOk,        ui->pbApply};

  ui->tbZoomIn->setToolTip(tr("Zoom in"));
  ui->tbZoomOut->setToolTip(tr("Zoom out"));
  ui->tbZoomReset->setToolTip(tr("Reset zoom"));

  ui->labelWarning->setToolTip(tr("Warning: Preview may be inaccurate (zoom factor has been modified)"));
  ui->tbAddFave->setToolTip(tr("Add fave"));

  ui->tbResetParameters->setToolTip(tr("Reset parameters to default values"));
  ui->tbResetParameters->setVisible(false);

  ui->tbUpdateFilters->setToolTip(tr("Update filters"));

  ui->tbRenameFave->setToolTip(tr("Rename fave"));
  ui->tbRenameFave->setEnabled(false);
  ui->tbRemoveFave->setToolTip(tr("Remove fave"));
  ui->tbRemoveFave->setEnabled(false);
  ui->pbFullscreen->setCheckable(true);

  ui->tbExpandCollapse->setToolTip(tr("Expand/Collapse all"));

  ui->logosLabel->setToolTip(tr("G'MIC (http://gmic.eu)<br/>"
                                "GREYC (http://www.greyc.fr)<br/>"
                                "CNRS (http://www.cnrs.fr)<br/>"
                                "Normandy University (http://www.unicaen.fr)<br/>"
                                "Ensicaen (http://www.ensicaen.fr)"));
  ui->logosLabel->setPixmap(QPixmap(":resources/logos.png"));

  ui->tbSelectionMode->setToolTip(tr("Selection mode"));
  ui->tbSelectionMode->setCheckable(true);

  _waitingCursorTimer.setSingleShot(true);
  connect(&_waitingCursorTimer, SIGNAL(timeout()), this, SLOT(showWaitingCursor()));

  ui->cbPreview->setChecked(true);

  _currentFiltersTreeModel = &_filtersTreeModel;

  ui->filterName->setTextFormat(Qt::RichText);
  ui->filterName->setVisible(false);

  ui->progressInfoWidget->hide();
  ui->messageLabel->setText(QString());
  ui->messageLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

  ui->filterParams->setNoFilter();
  _pendingActionAfterCurrentProcessing = NoAction;
  ui->inOutSelector->disable();
  ui->splitter->setChildrenCollapsible(false);

  QAction * searchAction = new QAction(this);
  searchAction->setShortcut(QKeySequence::Find);
  searchAction->setShortcutContext(Qt::ApplicationShortcut);
  connect(searchAction, SIGNAL(triggered(bool)), ui->searchField, SLOT(setFocus()));
  addAction(searchAction);

  ui->splitter->setHandleWidth(6);
  ui->verticalSplitter->setHandleWidth(6);
  ui->verticalSplitter->setStretchFactor(0, 5);
  ui->verticalSplitter->setStretchFactor(0, 1);

  QPalette p = qApp->palette();
  DialogSettings::UnselectedFilterTextColor = p.color(QPalette::Disabled, QPalette::WindowText);

  _filtersPresenter = new FiltersPresenter(this);
  _filtersPresenter->setFiltersView(ui->filtersView);

  loadSettings();

  ParametersCache::load(!_newSession);

  setIcons();

  QAction * escAction = new QAction(this);
  escAction->setShortcut(QKeySequence(Qt::Key_Escape));
  escAction->setShortcutContext(Qt::ApplicationShortcut);
  connect(escAction, SIGNAL(triggered(bool)), this, SLOT(onEscapeKeyPressed()));
  addAction(escAction);

  LayersExtentProxy::clearCache();
  QSize layersExtents = LayersExtentProxy::getExtent(ui->inOutSelector->inputMode());
  ui->previewWidget->setFullImageSize(layersExtents);
  makeConnections();

  _previewRandomSeed = cimg_library::cimg::srand();
}

MainWindow::~MainWindow()
{
  //  QSet<QString> hashes;
  //  FiltersTreeAbstractItem::buildHashesList(_filtersTreeModel.invisibleRootItem(),hashes);
  //  ParametersCache::cleanup(hashes);

  saveCurrentParameters();
  ParametersCache::save();
  saveSettings();
  Logger::setMode(Logger::StandardOutput); // Close log file, if necessary
  delete ui;
  delete _gmicImages;
  if (_unfinishedAbortedThreads.size()) {
    qWarning() << QString("Error: ~MainWindow(): There are %1 unfinished filter threads.").arg(_unfinishedAbortedThreads.size());
  }
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
  ui->tbSelectionMode->setIcon(LOAD_ICON("selection_mode"));
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
  p.setColor(QPalette::Window, QColor(53, 53, 53));
  p.setColor(QPalette::Button, QColor(73, 73, 73));
  p.setColor(QPalette::Highlight, QColor(110, 110, 110));
  p.setColor(QPalette::Text, QColor(255, 255, 255));
  p.setColor(QPalette::ButtonText, QColor(255, 255, 255));
  p.setColor(QPalette::WindowText, QColor(255, 255, 255));
  QColor linkColor(100, 100, 100);
  linkColor = linkColor.lighter();
  p.setColor(QPalette::Link, linkColor);
  p.setColor(QPalette::LinkVisited, linkColor);

  p.setColor(QPalette::Disabled, QPalette::Button, QColor(53, 53, 53));
  p.setColor(QPalette::Disabled, QPalette::Window, QColor(53, 53, 53));
  p.setColor(QPalette::Disabled, QPalette::Text, QColor(110, 110, 110));
  p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(110, 110, 110));
  p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(110, 110, 110));
  qApp->setPalette(p);

  p = ui->cbInternetUpdate->palette();
  p.setColor(QPalette::Text, DialogSettings::CheckBoxTextColor);
  p.setColor(QPalette::Base, DialogSettings::CheckBoxBaseColor);
  ui->cbInternetUpdate->setPalette(p);
  ui->cbPreview->setPalette(p);

  const QString css = "QTreeView { background: #505050; }"
                      "QLineEdit { background: #505050; }"
                      "QTextEdit { background: #505050; }"
                      "QSpinBox  { background: #505050; }"
                      "QDoubleSpinBox { background: #505050; }"
                      "QToolButton:checked { background: #383838; }"
                      "QToolButton:pressed { background: #383838; }"
                      "QComboBox QAbstractItemView { background: #505050; } "
                      "QFileDialog QAbstractItemView { background: #505050; } "
                      "QComboBox:editable { background: #505050; } "
                      "QProgressBar { background: #505050; }";
  qApp->setStyleSheet(css);
  ui->inOutSelector->setDarkTheme();

  DialogSettings::UnselectedFilterTextColor = DialogSettings::UnselectedFilterTextColor.darker(150);
}

void MainWindow::updateFiltersFromSources(int ageLimit, bool useNetwork)
{
  connect(Updater::getInstance(), SIGNAL(downloadsFinished(bool)), this, SLOT(onUpdateDownloadsFinished(bool)), Qt::UniqueConnection);
  Updater::getInstance()->startUpdate(ageLimit, 60, useNetwork);
}

void MainWindow::onUpdateDownloadsFinished(bool ok)
{
  if (!ok) {
    showUpdateErrors();
  } else {
    if (ui->cbInternetUpdate->isChecked()) {
      QMessageBox::information(this, tr("Update completed"), tr("Filter definitions have been updated."));
    } else {
      showMessage(tr("Filter definitions have been updated."), 3000);
    }
  }

  buildFiltersTree();

  ui->tbUpdateFilters->setEnabled(true);
  if (!_filtersPresenter->currentFilter().hash.isEmpty()) {
    ui->previewWidget->sendUpdateRequest();
  }
}

void MainWindow::buildFiltersTree()
{
  saveCurrentParameters();
  GmicStdLib::Array = Updater::getInstance()->buildFullStdlib();
  const bool withVisibility = filtersSelectionMode();
  //  GmicStdLibParser::buildFiltersTree(_filtersTreeModel,withVisibility);

  // TODO : Is this the right place?
  _filtersPresenter->clear();
  _filtersPresenter->readFilters();
  _filtersPresenter->readFaves();
  if (_gtkFavesShouldBeImported) {
    _filtersPresenter->importGmicGTKFaves();
    _filtersPresenter->saveFaves();
    _gtkFavesShouldBeImported = false;
    QSettings().setValue(FAVES_IMPORT_KEY, true);
  }

  QString searchText = ui->searchField->text();
  _filtersPresenter->toggleSelectionMode(withVisibility);
  _filtersPresenter->applySearchCriterion(searchText);

  if (_filtersPresenter->currentFilter().hash.isEmpty()) {
    setNoFilter();
    ui->previewWidget->sendUpdateRequest();
  } else {
    activateFilter(false);
  }
}

void MainWindow::startupUpdateFinished(bool ok)
{
  QObject::disconnect(Updater::getInstance(), SIGNAL(downloadsFinished(bool)), this, SLOT(startupUpdateFinished(bool)));
  if (_showMaximized) {
    show();
    showMaximized();
  } else {
    show();
  }
  if (!ok) {
    showUpdateErrors();
  } else if (Updater::getInstance()->someNetworkUpdateAchieved()) {
    showMessage(tr("Filter definitions have been updated"), 4000);
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
  const FiltersPresenter::Filter & currentFilter = _filtersPresenter->currentFilter();
  if (!currentFilter.hash.isEmpty() && !currentFilter.isAccurateIfZoomed && !ui->previewWidget->isAtDefaultZoom()) {
    ui->labelWarning->setPixmap(QPixmap(":/images/warning.png"));
  } else {
    ui->labelWarning->setPixmap(QPixmap(":/images/no_warning.png"));
  }
}

void MainWindow::updateZoomLabel(double zoom)
{
  ui->zoomLevelSelector->display(zoom);
}

void MainWindow::onFiltersSelectionModeToggled(bool on)
{
  _filtersPresenter->toggleSelectionMode(on);
  _filtersPresenter->applySearchCriterion(ui->searchField->text());
}

void MainWindow::onPreviewCheckBoxToggled(bool on)
{
  if (!on && _filterThread) {
    abortCurrentFilterThread();
  }
  ui->previewWidget->enablePreview(on);
}

void MainWindow::onFilterSelectionChanged()
{
  activateFilter(false);
  ui->previewWidget->sendUpdateRequest();
}

void MainWindow::onEscapeKeyPressed()
{
  ui->searchField->clear();
  if (_filterThread) {
    abortCurrentFilterThread();
    ui->previewWidget->displayOriginalImage();
  }
}

void MainWindow::clearMessage()
{
  if (!_messageTimerID) {
    return;
  }
  killTimer(_messageTimerID);
  ui->messageLabel->setText(QString());
  _messageTimerID = 0;
}

void MainWindow::timerEvent(QTimerEvent * e)
{
  if (e->timerId() == _messageTimerID) {
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
  for (QString s : errors) {
    message += QString("<br/>%1").arg(s);
  }
  QMessageBox::information(this, tr("Update error"), message);
}

void MainWindow::makeConnections()
{
  connect(ui->zoomLevelSelector, SIGNAL(valueChanged(double)), ui->previewWidget, SLOT(setZoomLevel(double)));

  connect(ui->previewWidget, SIGNAL(zoomChanged(double)), this, SLOT(showZoomWarningIfNeeded()));
  connect(ui->previewWidget, SIGNAL(zoomChanged(double)), this, SLOT(updateZoomLabel(double)));

  connect(_filtersPresenter, SIGNAL(filterSelectionChanged()), this, SLOT(onFilterSelectionChanged()));

  connect(ui->pbOk, SIGNAL(clicked(bool)), this, SLOT(onOkClicked()));
  connect(ui->pbCancel, SIGNAL(clicked(bool)), this, SLOT(onCloseClicked()));
  connect(ui->pbApply, SIGNAL(clicked(bool)), this, SLOT(onApplyClicked()));
  connect(ui->tbResetParameters, SIGNAL(clicked(bool)), this, SLOT(onReset()));

  connect(ui->tbUpdateFilters, SIGNAL(clicked(bool)), this, SLOT(onUpdateFiltersClicked()));

  connect(ui->pbSettings, SIGNAL(clicked(bool)), this, SLOT(onSettingsClicked()));

  connect(ui->pbFullscreen, SIGNAL(toggled(bool)), this, SLOT(onToggleFullScreen(bool)));

  connect(ui->filterParams, SIGNAL(valueChanged()), ui->previewWidget, SLOT(sendUpdateRequest()));

  connect(ui->previewWidget, SIGNAL(previewUpdateRequested()), this, SLOT(onPreviewUpdateRequested()));

  connect(ui->tbZoomIn, SIGNAL(clicked(bool)), this, SLOT(onZoomIn()));
  connect(ui->tbZoomOut, SIGNAL(clicked(bool)), this, SLOT(onZoomOut()));
  connect(ui->tbZoomReset, SIGNAL(clicked(bool)), this, SLOT(onPreviewZoomReset()));

  connect(ui->tbAddFave, SIGNAL(clicked(bool)), this, SLOT(onAddFave()));
  connect(ui->tbRemoveFave, SIGNAL(clicked(bool)), this, SLOT(onRemoveFave()));
  connect(ui->tbRenameFave, SIGNAL(clicked(bool)), this, SLOT(onRenameFave()));

  connect(ui->inOutSelector, SIGNAL(inputModeChanged(GmicQt::InputMode)), ui->previewWidget, SLOT(sendUpdateRequest()));
  connect(ui->inOutSelector, SIGNAL(outputMessageModeChanged(GmicQt::OutputMessageMode)), this, SLOT(onOutputMessageModeChanged(GmicQt::OutputMessageMode)));
  connect(ui->inOutSelector, SIGNAL(previewModeChanged(GmicQt::PreviewMode)), ui->previewWidget, SLOT(sendUpdateRequest()));

  connect(ui->cbPreview, SIGNAL(toggled(bool)), this, SLOT(onPreviewCheckBoxToggled(bool)));

  connect(ui->searchField, SIGNAL(textChanged(QString)), this, SLOT(search(QString)));

  connect(ui->tbExpandCollapse, SIGNAL(clicked(bool)), this, SLOT(expandOrCollapseFolders()));
  connect(ui->progressInfoWidget, SIGNAL(cancel()), this, SLOT(onCancelProcess()));

  connect(ui->tbSelectionMode, SIGNAL(toggled(bool)), this, SLOT(onFiltersSelectionModeToggled(bool)));
}

void MainWindow::onPreviewUpdateRequested()
{
  if (!ui->cbPreview->isChecked()) {
    ui->previewWidget->invalidateSavedPreview();
    return;
  }
  if (_filterThread) {
    abortCurrentFilterThread();
  }
  if (_filtersPresenter->currentFilter().isNoFilter()) {
    ui->previewWidget->displayOriginalImage();
    return;
  }
  ui->tbUpdateFilters->setEnabled(false);
  _gmicImages->assign(1);
  double x, y, w, h;
  ui->previewWidget->normalizedVisibleRect(x, y, w, h);
  GmicQt::InputMode inputMode = ui->inOutSelector->inputMode();
  gmic_list<char> imageNames;
  gmic_qt_get_cropped_images(*_gmicImages, imageNames, x, y, w, h, inputMode);
  ui->previewWidget->updateImageNames(imageNames, inputMode);
  double zoomFactor = ui->previewWidget->currentZoomFactor();
  if (zoomFactor < 1.0) {
    for (unsigned int i = 0; i < _gmicImages->size(); ++i) {
      gmic_image<float> & image = (*_gmicImages)[i];
      image.resize(std::round(image.width() * zoomFactor), std::round(image.height() * zoomFactor), 1, -100, 1);
    }
  }
  QString env = ui->inOutSelector->gmicEnvString();
  env += QString(" _preview_width=%1 _preview_height=%2").arg(ui->previewWidget->width()).arg(ui->previewWidget->height());
  const int timeout = DialogSettings::previewTimeout();
  env += QString(" _preview_timeout=%1").arg(timeout);
  const FiltersPresenter::Filter currentFilter = _filtersPresenter->currentFilter();
  _filterThread = new FilterThread(this, currentFilter.plainTextName, currentFilter.previewCommand, ui->filterParams->valueString(), env, ui->inOutSelector->outputMessageMode());
  _filterThread->setInputImages(*_gmicImages, imageNames);
  connect(_filterThread, SIGNAL(finished()), this, SLOT(onPreviewThreadFinished()));
  ui->filterParams->clearButtonParameters();
  _waitingCursorTimer.start(WAITING_CURSOR_DELAY);
  _okButtonShouldApply = true;
  _previewRandomSeed = cimg_library::cimg::srand();
  _filterThread->start();
}

void MainWindow::onPreviewThreadFinished()
{
  if (!_filterThread) {
    ui->tbUpdateFilters->setEnabled(true);
    return;
  }
  if (_pendingActionAfterCurrentProcessing == CloseAction) {
    _filterThread->deleteLater();
    close();
  }
  QStringList list = _filterThread->gmicStatus();
  if (!list.isEmpty()) {
    ui->filterParams->setValues(list, false);
  }
  if (_filterThread->failed()) {
    gmic_image<float> image = ui->previewWidget->image();
    QSize size = QSize(image.width(), image.height()).scaled(ui->previewWidget->size(), Qt::KeepAspectRatio);
    image.resize(size.width(), size.height(), 1, -100, 1);
    QImage qimage;
    ImageConverter::convert(image, qimage);
    QPainter painter(&qimage);
    painter.fillRect(qimage.rect(), QColor(40, 40, 40, 200));
    painter.setPen(Qt::green);
    painter.drawText(qimage.rect(), Qt::AlignCenter | Qt::TextWordWrap, _filterThread->errorMessage());
    painter.end();
    ImageConverter::convert(qimage, image);
    ui->previewWidget->setPreviewImage(image);
  } else {
    gmic_list<gmic_pixel_type> images = _filterThread->images();
    for (unsigned int i = 0; i < images.size(); ++i) {
      gmic_qt_apply_color_profile(images[i]);
    }
    gmic_image<float> image;
    GmicQt::buildPreviewImage(images, image, ui->inOutSelector->previewMode(), ui->previewWidget->width(), ui->previewWidget->height());
    ui->previewWidget->setPreviewImage(image);
  }

  _waitingCursorTimer.stop();
  if (QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::WaitCursor) {
    QApplication::restoreOverrideCursor();
  }
  _filterThread->deleteLater();
  _filterThread = 0;
  ui->tbUpdateFilters->setEnabled(true);
}

void MainWindow::processImage()
{
  // Abort any already running thread
  if (_filterThread) {
    abortCurrentFilterThread();
  }
  if (_filtersPresenter->currentFilter().isNoFilter()) {
    return;
  }
  _gmicImages->assign();
  gmic_list<char> imageNames;
  gmic_qt_get_cropped_images(*_gmicImages, imageNames, -1, -1, -1, -1, ui->inOutSelector->inputMode());
  ui->filterParams->updateValueString(false); // Required to get up-to-date values of text parameters
  const FiltersPresenter::Filter currentFilter = _filtersPresenter->currentFilter();
  _filterThread = new FilterThread(this, _lastFilterName = currentFilter.plainTextName, _lastAppliedCommand = currentFilter.command, _lastAppliedCommandArguments = ui->filterParams->valueString(),
                                   ui->inOutSelector->gmicEnvString(), _lastAppliedCommandOutputMessageMode = ui->inOutSelector->outputMessageMode());
  _filterThread->setInputImages(*_gmicImages, imageNames);
  connect(_filterThread, SIGNAL(finished()), this, SLOT(onApplyThreadFinished()));
  _waitingCursorTimer.start(WAITING_CURSOR_DELAY);
  ui->progressInfoWidget->startAnimationAndShow(_filterThread, true);
  ui->filterParams->clearButtonParameters();

  // Disable most of the GUI
  for (QWidget * w : _filterUpdateWidgets) {
    w->setEnabled(false);
  }

  cimg_library::cimg::srand(_previewRandomSeed);
  _filterThread->start();
}

void MainWindow::onApplyThreadFinished()
{
  ui->progressInfoWidget->stopAnimationAndHide();
  // Re-enable the GUI
  for (QWidget * w : _filterUpdateWidgets) {
    w->setEnabled(true);
  }
  ui->previewWidget->update();

  QStringList list = _filterThread->gmicStatus();
  if (!list.isEmpty()) {
    ui->filterParams->setValues(list, false);
  }

  if (QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::WaitCursor) {
    QApplication::restoreOverrideCursor();
  }
  _waitingCursorTimer.stop();

  if (_filterThread->failed()) {
    _lastAppliedCommand.clear();
    _lastFilterName.clear();
    _lastAppliedCommandArguments.clear();
    _lastAppliedCommandOutputMessageMode = GmicQt::Quiet;
    QMessageBox::warning(this, tr("Error"), _filterThread->errorMessage(), QMessageBox::Close);
  } else {
    gmic_list<gmic_pixel_type> images = _filterThread->images();
    if ((_pendingActionAfterCurrentProcessing == OkAction || _pendingActionAfterCurrentProcessing == ApplyAction) && !_filterThread->aborted()) {
      SHOW(_filterThread);
      gmic_qt_output_images(
          images, _filterThread->imageNames(), ui->inOutSelector->outputMode(),
          (ui->inOutSelector->outputMessageMode() == GmicQt::VerboseLayerName) ? QString("[G'MIC] %1: %2").arg(_filterThread->name()).arg(_filterThread->fullCommand()).toLocal8Bit().constData() : 0);
    }
  }
  _filterThread->deleteLater();
  _filterThread = 0;
  if ((_pendingActionAfterCurrentProcessing == OkAction || _pendingActionAfterCurrentProcessing == CloseAction)) {
    close();
  } else {
    LayersExtentProxy::clearCache();
    QSize extent = LayersExtentProxy::getExtent(ui->inOutSelector->inputMode());
    ui->previewWidget->setFullImageSize(extent);
    ui->previewWidget->sendUpdateRequest();
    _okButtonShouldApply = false;
  }
}

void MainWindow::showWaitingCursor()
{
  if (_filterThread && !(QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::WaitCursor)) {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  }
}

void MainWindow::expandOrCollapseFolders()
{
  if (_expandCollapseIcon == &_expandIcon) {
    _filtersPresenter->expandAll();
    ui->tbExpandCollapse->setIcon(_collapseIcon);
    _expandCollapseIcon = &_collapseIcon;
  } else {
    ui->tbExpandCollapse->setIcon(_expandIcon);
    _filtersPresenter->collapseAll();
    _expandCollapseIcon = &_expandIcon;
  }
}

void MainWindow::search(QString text)
{
  _filtersPresenter->applySearchCriterion(text);
}

void MainWindow::onApplyClicked()
{
  _pendingActionAfterCurrentProcessing = ApplyAction;
  processImage();
}

void MainWindow::onOkClicked()
{
  if (_filtersPresenter->currentFilter().isNoFilter()) {
    close();
  }
  if (_okButtonShouldApply) {
    _pendingActionAfterCurrentProcessing = OkAction;
    processImage();
  } else {
    close();
  }
}

void MainWindow::onCloseClicked()
{
  if (_filterThread && confirmAbortProcessingOnCloseRequest()) {
    _pendingActionAfterCurrentProcessing = CloseAction;
    if (_filterThread) {
      _filterThread->abortGmic();
    }
  } else {
    close();
  }
}

void MainWindow::onCancelProcess()
{
  if (_filterThread && _filterThread->isRunning()) {
    _pendingActionAfterCurrentProcessing = NoAction;
    _filterThread->abortGmic();
  }
}

void MainWindow::onReset()
{
  if (_filtersPresenter->currentFilter().hash.isEmpty() && _filtersPresenter->currentFilter().isAFave) {
    ui->filterParams->setValues(_filtersPresenter->currentFilter().defaultParameterValues, true);
    return;
  }
  if (not _filtersPresenter->currentFilter().isNoFilter()) {
    ui->filterParams->reset(true);
  }
}

void MainWindow::onPreviewZoomReset()
{
  if (!_filtersPresenter->currentFilter().hash.isEmpty()) {
    ui->previewWidget->setPreviewFactor(_filtersPresenter->currentFilter().previewFactor, true);
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
  QString hash = ui->filterParams->filterHash();
  if (!hash.isEmpty()) {
    ParametersCache::setValues(hash, ui->filterParams->valueStringList());
    ParametersCache::setInputOutputState(hash, ui->inOutSelector->state());
  }
}

void MainWindow::saveSettings()
{
  QSettings settings;

  _filtersPresenter->saveSettings(settings);

  // Cleanup obsolete keys
  settings.remove("OutputMessageModeIndex");
  settings.remove("OutputMessageModeValue");
  settings.remove("InputLayers");
  settings.remove("OutputMode");
  settings.remove("PreviewMode");

  // Save all settings

  DialogSettings::saveSettings(settings);
  settings.setValue("LastExecution/gmic_version", gmic_version);
  settings.setValue(QString("LastExecution/host_%1/Command").arg(GmicQt::HostApplicationShortname), _lastAppliedCommand);
  settings.setValue(QString("LastExecution/host_%1/FilterName").arg(GmicQt::HostApplicationShortname), _lastFilterName);
  settings.setValue(QString("LastExecution/host_%1/Arguments").arg(GmicQt::HostApplicationShortname), _lastAppliedCommandArguments);
  settings.setValue(QString("LastExecution/host_%1/OutputMessageMode").arg(GmicQt::HostApplicationShortname), _lastAppliedCommandOutputMessageMode);
  settings.setValue(QString("LastExecution/host_%1/InputMode").arg(GmicQt::HostApplicationShortname), ui->inOutSelector->inputMode());
  settings.setValue(QString("LastExecution/host_%1/OutputMode").arg(GmicQt::HostApplicationShortname), ui->inOutSelector->outputMode());
  settings.setValue(QString("LastExecution/host_%1/PreviewMode").arg(GmicQt::HostApplicationShortname), ui->inOutSelector->previewMode());
  settings.setValue(QString("LastExecution/host_%1/GmicEnvironment").arg(GmicQt::HostApplicationShortname), ui->inOutSelector->gmicEnvString());
  settings.setValue("SelectedFilter", _filtersPresenter->currentFilter().hash);
  settings.setValue("Config/MainWindowPosition", frameGeometry().topLeft());
  settings.setValue("Config/MainWindowRect", rect());
  settings.setValue("Config/MainWindowMaximized", isMaximized());
  settings.setValue("Config/ShowAllFilters", filtersSelectionMode());
  settings.setValue("LastExecution/ExitedNormally", true);
  settings.setValue("LastExecution/HostApplicationID", GmicQt::host_app_pid());
  QList<int> splitterSizes = ui->splitter->sizes();
  for (int i = 0; i < splitterSizes.size(); ++i) {
    settings.setValue(QString("Config/PanelSize%1").arg(i), splitterSizes.at(i));
  }
  splitterSizes = ui->verticalSplitter->sizes();
  settings.setValue(QString("Config/VerticalSplitterSizeTop"), splitterSizes.at(0));
  settings.setValue(QString("Config/VerticalSplitterSizeBottom"), splitterSizes.at(1));
  settings.setValue(REFRESH_USING_INTERNET_KEY, ui->cbInternetUpdate->isChecked());
  settings.remove("Config/VerticalSplitterSize0");
  settings.remove("Config/VerticalSplitterSize1");
}

void MainWindow::loadSettings()
{
  QSettings settings;
  DialogSettings::loadSettings();
  _filtersPresenter->loadSettings(settings);

  _lastExecutionOK = settings.value("LastExecution/ExitedNormally", true).toBool();
  _newSession = GmicQt::host_app_pid() != settings.value("LastExecution/HostApplicationID", 0).toUInt();
  settings.setValue("LastExecution/ExitedNormally", false);
  ui->inOutSelector->reset();

  // Preview position
  if (settings.value("Config/PreviewPosition", "Left").toString() == "Left") {
    setPreviewPosition(PreviewOnLeft);
  }
  if (DialogSettings::darkThemeEnabled()) {
    setDarkTheme();
  }
  if (!DialogSettings::logosAreVisible()) {
    ui->logosLabel->hide();
  }

  // Mainwindow geometry
  QPoint position = settings.value("Config/MainWindowPosition", QPoint()).toPoint();
  QRect r = settings.value("Config/MainWindowRect", QRect()).toRect();
  _showMaximized = settings.value("Config/MainWindowMaximized", false).toBool();
  if (_showMaximized) {
    ui->pbFullscreen->setChecked(true);
  } else {
    if (r.isValid()) {
      setGeometry(r);
      move(position);
    } else {
      QDesktopWidget desktop;
      QRect screenSize = desktop.availableGeometry();
      screenSize.setWidth(screenSize.width() * 0.66);
      screenSize.setHeight(screenSize.height() * 0.66);
      screenSize.moveCenter(desktop.availableGeometry().center());
      setGeometry(screenSize);
      int w = screenSize.width();
      ui->splitter->setSizes(QList<int>() << (w * 0.4) << (w * 0.2) << (w * 0.4));
    }
  }

  // Splitter sizes
  QList<int> sizes;
  for (int i = 0; i < 3; ++i) {
    int s = settings.value(QString("Config/PanelSize%1").arg(i), 0).toInt();
    if (s) {
      sizes.push_back(s);
    }
  }
  if (sizes.size() == 3) {
    ui->splitter->setSizes(sizes);
  }

  // Filters visibility
  ui->tbSelectionMode->setChecked(settings.value("Config/ShowAllFilters", false).toBool());
  ui->cbInternetUpdate->setChecked(settings.value("Config/RefreshInternetUpdate", true).toBool());
}

void MainWindow::setPreviewPosition(MainWindow::PreviewPosition position)
{
  if (position == _previewPosition) {
    return;
  }
  _previewPosition = position;

  QHBoxLayout * layout = dynamic_cast<QHBoxLayout *>(ui->belowPreviewWidget->layout());
  if (layout) {
    // layout->removeWidget(ui->inOutSelector);
    layout->removeWidget(ui->belowPreviewPadding);
    layout->removeWidget(ui->logosLabel);
    if (position == MainWindow::PreviewOnLeft) {
      layout->addWidget(ui->logosLabel);
      layout->addWidget(ui->belowPreviewPadding);
      // layout->addWidget(ui->inOutSelector);
    } else {
      // layout->addWidget(ui->inOutSelector);
      layout->addWidget(ui->belowPreviewPadding);
      layout->addWidget(ui->logosLabel);
    }
  }

  // Swap splitter widgets
  QWidget * preview;
  QWidget * list;
  QWidget * params;
  if (position == MainWindow::PreviewOnRight) {
    ui->messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    preview = ui->splitter->widget(0);
    list = ui->splitter->widget(1);
    params = ui->splitter->widget(2);
  } else {
    ui->messageLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    list = ui->splitter->widget(0);
    params = ui->splitter->widget(1);
    preview = ui->splitter->widget(2);
  }
  preview->hide();
  list->hide();
  params->hide();
  preview->setParent(this);
  list->setParent(this);
  params->setParent(this);
  if (position == MainWindow::PreviewOnRight) {
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
  ui->logosLabel->setAlignment(Qt::AlignVCenter | ((_previewPosition == PreviewOnRight) ? Qt::AlignRight : Qt::AlignLeft));
}

void MainWindow::abortCurrentFilterThread()
{
  _filterThread->disconnect(this);
  connect(_filterThread, SIGNAL(finished()), this, SLOT(onAbortedThreadFinished()));
  _unfinishedAbortedThreads.push_back(_filterThread);
  _filterThread->abortGmic();
  _filterThread = 0;
  _waitingCursorTimer.stop();
  if (QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::WaitCursor) {
    QApplication::restoreOverrideCursor();
  }
}

void MainWindow::adjustVerticalSplitter()
{
  QList<int> sizes;
  QSettings settings;
  sizes.push_back(settings.value(QString("Config/VerticalSplitterSizeTop"), -1).toInt());
  sizes.push_back(settings.value(QString("Config/VerticalSplitterSizeBottom"), -1).toInt());
  if ((sizes.front() != -1) && (sizes.back() != -1)) {
    ui->verticalSplitter->setSizes(sizes);
  } else {
    int h1 = std::max(ui->inOutSelector->sizeHint().height(), 75);
    int h = std::max(ui->verticalSplitter->height(), 150);
    sizes.clear();
    sizes.push_back(h - h1);
    sizes.push_back(h1);
    ui->verticalSplitter->setSizes(sizes);
  }
}

void MainWindow::onAbortedThreadFinished()
{
  FilterThread * thread = dynamic_cast<FilterThread *>(sender());
  if (_unfinishedAbortedThreads.contains(thread)) {
    _unfinishedAbortedThreads.removeOne(thread);
    thread->deleteLater();
  }
}

bool MainWindow::filtersSelectionMode()
{
  return ui->tbSelectionMode->isChecked();
}

void MainWindow::activateFilter(bool resetZoom)
{
  saveCurrentParameters();
  const FiltersPresenter::Filter & filter = _filtersPresenter->currentFilter();
  if (filter.hash.isEmpty()) {
    setNoFilter();
  } else {
    QList<QString> savedValues = ParametersCache::getValues(filter.hash);
    if (savedValues.isEmpty() && filter.isAFave) {
      savedValues = filter.defaultParameterValues;
    }
    if (not ui->filterParams->build(filter.name, filter.hash, filter.parameters, savedValues)) {
      _filtersPresenter->setInvalidFilter();
    }
    ui->filterName->setText(QString("<b>%1</b>").arg(filter.name));
    ui->inOutSelector->enable();
    ui->inOutSelector->show();
    ui->inOutSelector->setState(ParametersCache::getInputOutputState(filter.hash), false);
    Logger::setMode(ui->inOutSelector->outputMessageMode());
    ui->filterName->setVisible(true);
    ui->tbAddFave->setEnabled(true);
    ui->previewWidget->setPreviewFactor(filter.previewFactor, resetZoom);
    ui->previewWidget->enableRightClick();
    showZoomWarningIfNeeded();
    _okButtonShouldApply = true;
    ui->tbResetParameters->setVisible(true);
    ui->tbRemoveFave->setEnabled(filter.isAFave);
    ui->tbRenameFave->setEnabled(filter.isAFave);
  }
}

void MainWindow::setNoFilter()
{
  ui->filterParams->setNoFilter();
  ui->previewWidget->disableRightClick();
  ui->inOutSelector->hide();
  ui->inOutSelector->setState(GmicQt::InputOutputState::Default, false);
  ui->filterName->setVisible(false);
  ui->tbAddFave->setEnabled(false);
  ui->tbResetParameters->setVisible(false);
  ui->labelWarning->setPixmap(QPixmap(":/images/no_warning.png"));
  _okButtonShouldApply = false;

  ui->tbRemoveFave->setEnabled(false);
  ui->tbRenameFave->setEnabled(false);
}

void MainWindow::showEvent(QShowEvent * event)
{
  static bool first = true;
  event->accept();
  if (!first) {
    return;
  }
  first = false;
  ui->searchField->setFocus();
  adjustVerticalSplitter();

  if (_newSession) {
    Logger::clear();
  }

  if (QSettings().value(FAVES_IMPORT_KEY, false).toBool() || !FavesModelReader::gmicGTKFaveFileAvailable()) {
    _gtkFavesShouldBeImported = false;
  } else {
    _gtkFavesShouldBeImported = askUserForGTKFavesImport();
  }
  buildFiltersTree();

  // Retrieve and select previously selected filter
  QString hash = QSettings().value("SelectedFilter", QString()).toString();
  if (_newSession || !_lastExecutionOK) {
    hash.clear();
  }
  _filtersPresenter->selectFilterFromHash(hash, false);
  if (_filtersPresenter->currentFilter().hash.isEmpty()) {
    _filtersPresenter->expandFaveFolder();
    ui->previewWidget->setPreviewFactor(GmicQt::PreviewFactorFullImage, true);
  } else {
    activateFilter(true);
  }
  _filtersPresenter->adjustViewSize();
  // Preview update is triggered when PreviewWidget receives
  // the WindowActivate Event (while pendingResize is true
  // after the very first resize event).
}

void MainWindow::resizeEvent(QResizeEvent * e)
{
  // Check if size is reducing
  if ((e->size().width() < e->oldSize().width() || e->size().height() < e->oldSize().height()) && ui->pbFullscreen->isChecked() && (windowState() & Qt::WindowMaximized)) {
    ui->pbFullscreen->toggle();
  }
}

bool MainWindow::askUserForGTKFavesImport()
{
  QMessageBox messageBox(QMessageBox::Question, tr("Import faves"), QString(tr("Do you want to import faves from file below?<br/>%1")).arg(FavesModelReader::gmicGTKFavesFilename()),
                         QMessageBox::Yes | QMessageBox::No, this);
  messageBox.setDefaultButton(QMessageBox::Yes);
  QCheckBox * cb = new QCheckBox(tr("Don't ask again"));
  if (DialogSettings::darkThemeEnabled()) {
    QPalette p = cb->palette();
    p.setColor(QPalette::Text, DialogSettings::CheckBoxTextColor);
    p.setColor(QPalette::Base, DialogSettings::CheckBoxBaseColor);
    cb->setPalette(p);
  }
  messageBox.setCheckBox(cb);
  int choice = messageBox.exec();
  if (choice != QMessageBox::Yes) {
    if (cb->isChecked()) {
      QSettings().setValue(FAVES_IMPORT_KEY, true);
    }
    return false;
  }
  return true;
}

void MainWindow::onAddFave()
{
  if (_filtersPresenter->currentFilter().hash.isEmpty()) {
    return;
  }
  saveCurrentParameters();
  _filtersPresenter->addSelectedFilterAsNewFave(ui->filterParams->valueStringList(), ui->inOutSelector->state());
}
void MainWindow::onRemoveFave()
{
  _filtersPresenter->removeSelectedFave();
}

void MainWindow::onRenameFave()
{
  _filtersPresenter->editSelectedFaveName();
}

void MainWindow::onOutputMessageModeChanged(GmicQt::OutputMessageMode mode)
{
  Logger::setMode(mode);
  ui->previewWidget->sendUpdateRequest();
}

void MainWindow::onToggleFullScreen(bool on)
{
  if (on && !(windowState() & Qt::WindowMaximized)) {
    showMaximized();
  }
  if (!on && (windowState() & Qt::WindowMaximized)) {
    showNormal();
  }
}

void MainWindow::onSettingsClicked()
{
  QList<int> splitterSizes = ui->splitter->sizes();

  int previewWidth;
  int paramsWidth;
  int treeWidth;
  if (_previewPosition == PreviewOnLeft) {
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
  if (previewPositionChanged) {
    splitterSizes.clear();
    if (_previewPosition == PreviewOnLeft) {
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
  if (DialogSettings::logosAreVisible()) {
    ui->logosLabel->show();
  } else {
    ui->logosLabel->hide();
  }
}

bool MainWindow::confirmAbortProcessingOnCloseRequest()
{
  int button = QMessageBox::question(this, tr("Confirmation"), tr("A gmic command is running.<br>Do you really want to close the plugin?"), QMessageBox::Yes, QMessageBox::No);
  return (button == QMessageBox::Yes);
}

void MainWindow::closeEvent(QCloseEvent * e)
{
  if (_filterThread && _pendingActionAfterCurrentProcessing != CloseAction) {
    if (confirmAbortProcessingOnCloseRequest()) {
      _filterThread->abortGmic();
      _pendingActionAfterCurrentProcessing = CloseAction;
    }
    e->ignore();
  } else {
    e->accept();
  }
}
