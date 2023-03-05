/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ZoomLevelSelector.cpp
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
#include "Widgets/ZoomLevelSelector.h"
#include <QDebug>
#include <QLineEdit>
#include <QRegularExpression>
#include <cmath>
#include "Common.h"
#include "Globals.h"
#include "IconLoader.h"
#include "PreviewWidget.h"
#include "ui_zoomlevelselector.h"

namespace GmicQt
{

ZoomLevelSelector::ZoomLevelSelector(QWidget * parent) : QWidget(parent), ui(new Ui::ZoomLevelSelector)
{
  ui->setupUi(this);
  _previewWidget = nullptr;

  ui->comboBox->setEditable(true);

  ui->comboBox->setInsertPolicy(QComboBox::NoInsert);
  ui->comboBox->setValidator(new ZoomLevelValidator(ui->comboBox));
  ui->comboBox->setCompleter(nullptr);
  _notificationsEnabled = true;

  ui->labelWarning->setPixmap(QPixmap(":/images/no_warning.png"));
  ui->labelWarning->setToolTip(QString());

  ui->tbZoomIn->setToolTip(tr("Zoom in"));
  ui->tbZoomOut->setToolTip(tr("Zoom out"));
  ui->tbZoomReset->setToolTip(tr("Reset zoom"));

  ui->tbZoomIn->setIcon(LOAD_ICON("zoom-in"));
  ui->tbZoomOut->setIcon(LOAD_ICON("zoom-out"));
  ui->tbZoomReset->setIcon(LOAD_ICON("view-refresh"));

  connect(ui->comboBox->lineEdit(), &QLineEdit::editingFinished, this, &ZoomLevelSelector::onComboBoxEditingFinished);
  connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ZoomLevelSelector::onComboIndexChanged);
  connect(ui->tbZoomIn, &QToolButton::clicked, this, &ZoomLevelSelector::zoomIn);
  connect(ui->tbZoomOut, &QToolButton::clicked, this, &ZoomLevelSelector::zoomOut);
  connect(ui->tbZoomReset, &QToolButton::clicked, this, &ZoomLevelSelector::zoomReset);
  setZoomConstraint(ZoomConstraint::Any);
}

ZoomLevelSelector::~ZoomLevelSelector()
{
  delete ui;
}

void ZoomLevelSelector::setZoomConstraint(const ZoomConstraint & constraint)
{
  _zoomConstraint = constraint;
  _notificationsEnabled = false;
  setEnabled(_zoomConstraint != ZoomConstraint::Fixed);
  double currentValue = currentZoomValue();

  QStringList values = {"1000 %", "800 %", "400 %", "200 %", "150 %", "100 %", "66.7 %", "50 %", "25 %", "12.5 %"};
  if (_zoomConstraint == ZoomConstraint::OneOrMore) {
    values.pop_back();
    values.pop_back();
    values.pop_back();
    values.pop_back();
    if (currentValue <= 1.0) {
      currentValue = 1.0;
    }
  }
  QString maxStr = values.front();
  maxStr.remove(" %");
  int max = maxStr.toInt();
  double previewMax = PREVIEW_MAX_ZOOM_FACTOR;
  while (max < previewMax * 100) {
    max += 1000;
    values.push_front(QString::number(max) + " %");
  }
  ui->comboBox->clear();
  ui->comboBox->addItems(values);

  display(currentValue);

  _notificationsEnabled = true;
}

void ZoomLevelSelector::setPreviewWidget(const PreviewWidget * widget)
{
  _previewWidget = widget;
}

void ZoomLevelSelector::display(const double zoom)
{
  bool decimals = static_cast<int>(zoom * 10000) % 100;
  QString text;
  if (decimals && zoom < 1) {
    text = QString("%L1 %").arg(zoom * 100.0, 0, 'f', 2);
  } else {
    text = QString("%1 %").arg(static_cast<int>(zoom * 100));
  }

  // Get closest proposed value in list
  double distanceMin = std::numeric_limits<double>::max();
  int iMin = 0;
  int count = ui->comboBox->count();
  for (int i = 0; i < count; ++i) {
    QString str = ui->comboBox->itemText(i);
    str.chop(2);
    double value = str.toDouble();
    double distance = std::fabs(value / 100.0 - zoom);
    if (distance < distanceMin) {
      distanceMin = distance;
      iMin = i;
    }
  }

  ui->tbZoomOut->setEnabled((!_previewWidget || !_previewWidget->isAtFullZoom()) && (((_zoomConstraint == ZoomConstraint::OneOrMore) && (zoom > 1.0)) || (_zoomConstraint == ZoomConstraint::Any)));

  if (_zoomConstraint == ZoomConstraint::Any || _zoomConstraint == ZoomConstraint::OneOrMore) {
    ui->tbZoomIn->setEnabled(zoom != PREVIEW_MAX_ZOOM_FACTOR);
  }

  _notificationsEnabled = false;
  ui->comboBox->setCurrentIndex(iMin);
  ui->comboBox->setEditText(text);
  _currentText = text;
  _notificationsEnabled = true;
}

void ZoomLevelSelector::showWarning(bool on)
{
  if (on) {
    ui->labelWarning->setPixmap(QPixmap(":/images/warning.png"));
    ui->labelWarning->setToolTip(tr("Warning: Preview may be inaccurate (zoom factor has been modified)"));
  } else {
    ui->labelWarning->setPixmap(QPixmap(":/images/no_warning.png"));
    ui->labelWarning->setToolTip(QString());
  }
}

void ZoomLevelSelector::onComboBoxEditingFinished()
{
  QString text = ui->comboBox->lineEdit()->text();
  if (text == _currentText) {
    // This is required because opening the combobox sends
    // an editingFinished() signal ;-(
    return;
  }
  if (!text.endsWith(" %")) {
    text.remove(QRegularExpression(" ?%?$"));
    text += " %";
  }
  QString digits = text;
  digits.remove(" %");
  double value = digits.toDouble();
  if ((_zoomConstraint == ZoomConstraint::OneOrMore) && (value < 100.0)) {
    ui->comboBox->lineEdit()->setText(_currentText = "100 %");
  } else {
    ui->comboBox->lineEdit()->setText(_currentText = text);
  }
  if (_notificationsEnabled) {
    emit valueChanged(currentZoomValue());
  }
}

void ZoomLevelSelector::onComboIndexChanged(int)
{
  _currentText = ui->comboBox->currentText();
  if (_notificationsEnabled) {
    emit valueChanged(currentZoomValue());
  }
}

double ZoomLevelSelector::currentZoomValue()
{
  QString text = ui->comboBox->currentText();
  text.remove(" %");
  return text.toDouble() / 100.0;
}

ZoomLevelValidator::ZoomLevelValidator(QObject * parent) : QValidator(parent)
{
  _doubleValidator = new QDoubleValidator(1e-10, PREVIEW_MAX_ZOOM_FACTOR * 100.0, 3, parent);
  _doubleValidator->setNotation(QDoubleValidator::StandardNotation);
}

QValidator::State ZoomLevelValidator::validate(QString & input, int & pos) const
{
  QString str = input;
  str.remove(QRegularExpression(" ?%?$"));
  return _doubleValidator->validate(str, pos);
}

} // namespace GmicQt
