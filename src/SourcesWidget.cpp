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
#include <QPushButton>
#include <QToolTip>
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
  ui->tbReset->setToolTip(tr("Set to default list"));
  connect(ui->tbOpen, &QPushButton::clicked, this, &SourcesWidget::onOpenFile);
  connect(ui->tbNew, &QPushButton::clicked, this, &SourcesWidget::onAddNew);
  connect(ui->tbReset, &QPushButton::clicked, this, &SourcesWidget::setToDefault);
  connect(ui->tbTrash, &QPushButton::clicked, this, &SourcesWidget::removeCurrentSource);
  connect(ui->tbUp, &QPushButton::clicked, this, &SourcesWidget::onMoveUp);
  connect(ui->tbDown, &QPushButton::clicked, this, &SourcesWidget::onMoveDown);
  connect(ui->list, &QListWidget::currentItemChanged, this, &SourcesWidget::enableButtons);
  connect(ui->list, &QListWidget::currentItemChanged, [this]() { //
    QListWidgetItem * item = ui->list->currentItem();
    if (item) {
      ui->leURL->setText(item->text());
    }
  });
  connect(ui->leURL, &QLineEdit::textChanged, [this](QString text) { //
    QListWidgetItem * item = ui->list->currentItem();
    if (item) {
      ui->list->currentItem()->setText(text);
    }
  });
  ui->list->addItems(Settings::filterSources());

#ifdef _IS_WINDOWS_
  ui->labelVariables->setText(tr("Macros: $HOME %APPDATA% $VERSION"));
#else
  ui->labelVariables->setText(tr("Macros: $HOME $VERSION"));
#endif

  ui->cbOfficialFilters->addItem(tr("Disable"), int(OfficialFilters::Disabled));
  ui->cbOfficialFilters->addItem(tr("Enable without updates"), int(OfficialFilters::EnabledWithoutUpdates));
  ui->cbOfficialFilters->addItem(tr("Enable with updates (recommended)"), int(OfficialFilters::EnabledWithUpdates));

  switch (Settings::officialFilterSource()) {
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
                                 "Furthermore, $VERSION is substituted by G'MIC version number (currently %1).")
                                  .arg(GmicQt::GmicVersion));
#else
  ui->labelVariables->setText(tr("Environment variables (e.g. $HOME or ${HOME} for your home directory) are substituted in sources.\n"
                                 "Furthermore, $VERSION is substituted by G'MIC version number (currently %1).")
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
    result.push_back(ui->list->item(row)->text());
  }
  return result;
}

QStringList SourcesWidget::defaultList()
{
  QStringList result;
#ifdef _IS_WINDOWS_
  result << QString("%APPDATA%%1user.gmic").arg(QDir::separator());
#else
  result << "${HOME}/.gmic";
#endif
  return result;
}

void SourcesWidget::saveSettings()
{
  Settings::setFilterSources(list());
  Settings::setOfficialFilterSource((OfficialFilters)ui->cbOfficialFilters->currentData().toInt());
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
}

void SourcesWidget::enableButtons()
{
  int index = ui->list->currentRow();
  TSHOW(index);
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
  SHOW(item);
  SHOW(row);
  if (item) {
    ui->list->removeItemWidget(item);
    delete item;
    if (ui->list->count()) {
      ui->list->setCurrentRow(std::min(ui->list->count() - 1, row));
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

} // namespace GmicQt
