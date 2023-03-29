/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file SourcesWidget.h
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
#include "SourcesWidget.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFileDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSet>
#include <QToolTip>
#include <QVector>
#include <algorithm>
#include "GmicStdlib.h"
#include "IconLoader.h"
#include "Settings.h"
#include "ui_sourceswidget.h"

namespace GmicQt
{

SourcesWidget::SourcesWidget(QWidget * parent) : QWidget(parent), ui(new Ui::SourcesWidget)
{
  ui->setupUi(this);

  ui->tbUp->setIcon(LOAD_ICON("draw-arrow-up"));
  ui->tbUp->setToolTip(tr("Move source up"));
  ui->tbDown->setIcon(LOAD_ICON("draw-arrow-down"));
  ui->tbDown->setToolTip(tr("Move source down"));
  ui->tbTrash->setIcon(LOAD_ICON("user-trash"));
  ui->tbTrash->setToolTip(tr("Remove source"));
  ui->tbOpen->setIcon(LOAD_ICON("folder"));
  ui->tbOpen->setToolTip(tr("Add local file (dialog)"));
  ui->tbReset->setIcon(LOAD_ICON("view-refresh"));
  ui->tbReset->setToolTip(tr("Reset filter sources"));
  connect(ui->tbOpen, &QPushButton::clicked, this, &SourcesWidget::onOpenFile);
  connect(ui->tbNew, &QPushButton::clicked, this, &SourcesWidget::onAddNew);
  connect(ui->tbReset, &QPushButton::clicked, this, &SourcesWidget::setToDefault);
  connect(ui->tbTrash, &QPushButton::clicked, this, &SourcesWidget::removeCurrentSource);
  connect(ui->tbUp, &QPushButton::clicked, this, &SourcesWidget::onMoveUp);
  connect(ui->tbDown, &QPushButton::clicked, this, &SourcesWidget::onMoveDown);
  connect(ui->list, &QListWidget::currentItemChanged, this, &SourcesWidget::onSourceSelected);
  connect(ui->leURL, &QLineEdit::textChanged, [this](QString text) { //
    QListWidgetItem * item = ui->list->currentItem();
    if (item) {
      ui->list->currentItem()->setText(text);
    }
  });
  ui->list->addItems(_sourcesAtOpening = Settings::filterSources());

#ifdef _IS_WINDOWS_
  ui->labelVariables->setText(tr("Macros: $HOME %APPDATA% $VERSION"));
#else
  ui->labelVariables->setText(tr("Macros: $HOME $VERSION"));
#endif

  ui->cbOfficialFilters->addItem(tr("Disable"), int(OfficialFilters::Disabled));
  ui->cbOfficialFilters->addItem(tr("Enable without updates"), int(OfficialFilters::EnabledWithoutUpdates));
  ui->cbOfficialFilters->addItem(tr("Enable with updates (recommended)"), int(OfficialFilters::EnabledWithUpdates));

  switch (_officialFiltersAtOpening = Settings::officialFilterSource()) {
  case OfficialFilters::Disabled:
    ui->cbOfficialFilters->setCurrentIndex(0);
    break;
  case OfficialFilters::EnabledWithoutUpdates:
    ui->cbOfficialFilters->setCurrentIndex(1);
    break;
  case OfficialFilters::EnabledWithUpdates:
    ui->cbOfficialFilters->setCurrentIndex(2);
    break;
  }

#ifdef _IS_WINDOWS_
  ui->labelVariables->setText(tr("Environment variables (e.g. %APPDATA% or %HOMEDIR%) are substituted in sources.\n"
                                 "VERSION is also a predefined variable that stands for the G'MIC version number (currently %1).")
                                  .arg(GmicQt::GmicVersion));
#else
  ui->labelVariables->setText(tr("Environment variables (e.g. $HOME or ${HOME} for your home directory) are substituted in sources.\n"
                                 "VERSION is also a predefined variable that stands for the G'MIC version number (currently %1).")
                                  .arg(GmicQt::GmicVersion));
#endif

  _newItemText = tr("New source");
  enableButtons();
}

SourcesWidget::~SourcesWidget()
{
  delete ui;
}

QStringList SourcesWidget::list() const
{
  QStringList result;
  const int count = ui->list->count();
  for (int row = 0; row < count; ++row) {
    QString text = ui->list->item(row)->text();
    if (!text.isEmpty() && (text != _newItemText)) {
      result.push_back(text);
    }
  }
  return result;
}

QStringList SourcesWidget::defaultList()
{
  QStringList result;
#ifdef _IS_WINDOWS_
  result << QString("%GMIC_USER%%1user.gmic").arg(QDir::separator());
  result << QString("%APPDATA%%1user.gmic").arg(QDir::separator());
#else
  result << "${GMIC_USER}/.gmic";
  result << "${HOME}/.gmic";
#endif
  return result;
}

void SourcesWidget::saveSettings()
{
  Settings::setFilterSources(list());
  Settings::setOfficialFilterSource((OfficialFilters)ui->cbOfficialFilters->currentData().toInt());
}

bool SourcesWidget::sourcesModified(bool & internetUpdateRequired)
{
  internetUpdateRequired = false;
  const QStringList currentSourceList = list();
  const OfficialFilters currentOfficialFilters = OfficialFilters(ui->cbOfficialFilters->currentData().toInt());
  if ((currentSourceList == _sourcesAtOpening) && (_officialFiltersAtOpening == currentOfficialFilters)) {
    return false;
  }
  QSet<QString> remoteSourcesBefore;
  for (const QString & source : _sourcesAtOpening) {
    if (source.startsWith("http://") || source.startsWith("https://")) {
      remoteSourcesBefore.insert(source);
    }
  }
  QSet<QString> remoteSourcesAfter;
  for (const QString & source : currentSourceList) {
    if (source.startsWith("http://") || source.startsWith("https://")) {
      remoteSourcesAfter.insert(source);
    }
  }
  if (!(remoteSourcesAfter - remoteSourcesBefore).isEmpty()) {
    internetUpdateRequired = true;
  }
  if ((currentOfficialFilters == OfficialFilters::EnabledWithUpdates) //
      && (currentOfficialFilters != _officialFiltersAtOpening)) {
    internetUpdateRequired = true;
  }
  return true;
}

void SourcesWidget::onOpenFile()
{
  const QFileDialog::Options options = Settings::nativeFileDialogs() ? QFileDialog::Options() : QFileDialog::DontUseNativeDialog;
  QString url = ui->leURL->text();
  QString folder;
  if (!url.isEmpty() && !url.startsWith("http://") && !url.startsWith("https://")) {
    folder = QFileInfo(url).absoluteDir().absolutePath();
  } else {
    folder = QDir::homePath();
  }
  QString filename = QFileDialog::getOpenFileName(this, tr("Select a file"), folder, QString(), nullptr, options);
  if (!filename.isEmpty()) {
    if (ui->leURL->text() == _newItemText) {
      ui->leURL->setText(filename);
    } else {
      ui->list->addItem(filename);
      ui->list->setCurrentRow(ui->list->count() - 1);
      enableButtons();
    }
  }
}

void SourcesWidget::onAddNew()
{
  ui->list->addItem(_newItemText);
  ui->list->setCurrentRow(ui->list->count() - 1);
  ui->leURL->selectAll();
  ui->leURL->setFocus();
}

void SourcesWidget::setToDefault()
{
  ui->list->clear();
  ui->list->addItems(defaultList());
  for (int i = 0; i < ui->cbOfficialFilters->count(); ++i) {
    if (ui->cbOfficialFilters->itemData(i).toInt() == int(OfficialFilters::EnabledWithUpdates)) {
      ui->cbOfficialFilters->setCurrentIndex(i);
      break;
    }
  }
}

void SourcesWidget::enableButtons()
{
  int index = ui->list->currentRow();
  if (index == -1) {
    ui->tbUp->setEnabled(false);
    ui->tbDown->setEnabled(false);
    ui->tbTrash->setEnabled(false);
    ui->leURL->clear();
    ui->leURL->setEnabled(false);
    return;
  }
  ui->tbUp->setEnabled(index > 0);
  ui->tbDown->setEnabled(index < ui->list->count() - 1);
  ui->tbTrash->setEnabled(true);
  ui->leURL->setEnabled(true);
}

void SourcesWidget::removeCurrentSource()
{
  QListWidgetItem * item = ui->list->currentItem();
  int row = ui->list->currentRow();
  if (item) {
    disconnect(ui->list, &QListWidget::currentItemChanged, this, nullptr);
    ui->list->removeItemWidget(item);
    delete item;
    connect(ui->list, &QListWidget::currentItemChanged, this, &SourcesWidget::onSourceSelected, Qt::UniqueConnection);
    if (ui->list->count()) {
      ui->list->setCurrentRow(std::min(ui->list->count() - 1, row));
      onSourceSelected();
    }
    enableButtons();
  }
}

void SourcesWidget::onMoveDown()
{
  int row = ui->list->currentRow();
  if (row >= ui->list->count() - 1) {
    return;
  }
  QString textDown = ui->list->item(row + 1)->text();
  ui->list->item(row + 1)->setText(ui->list->item(row)->text());
  ui->list->item(row)->setText(textDown);
  ui->list->setCurrentRow(row + 1);
}

void SourcesWidget::onMoveUp()
{
  int row = ui->list->currentRow();
  if (row < 1) {
    return;
  }
  QString textUp = ui->list->item(row - 1)->text();
  ui->list->item(row - 1)->setText(ui->list->item(row)->text());
  ui->list->item(row)->setText(textUp);
  ui->list->setCurrentRow(row - 1);
}

void SourcesWidget::onSourceSelected()
{
  enableButtons();
  cleanupEmptySources();
  QListWidgetItem * item = ui->list->currentItem();
  if (item) {
    ui->leURL->setText(item->text());
  }
}

void SourcesWidget::cleanupEmptySources()
{
  QListWidgetItem * currentItem = ui->list->currentItem();
  QVector<QListWidgetItem *> removableItems;
  for (int row = 0; row < ui->list->count(); ++row) {
    QListWidgetItem * item = ui->list->item(row);
    if (item && (item != currentItem) && (item->text().isEmpty() || (item->text() == _newItemText))) {
      removableItems.push_back(item);
    }
  }
  for (QListWidgetItem * item : removableItems) {
    ui->list->removeItemWidget(item);
    delete item;
  }
  if (currentItem) {
    for (int row = 0; row < ui->list->count(); ++row) {
      if (ui->list->item(row) == currentItem) {
        ui->list->setCurrentRow(row);
        break;
      }
    }
  }
}

} // namespace GmicQt
