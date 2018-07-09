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
#include <cmath>
#include "Common.h"
#include "DialogSettings.h"
#include "Globals.h"
#include "PreviewWidget.h"
#include "ui_zoomlevelselector.h"

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

  QPixmap pixZoomIn = LOAD_PIXMAP("zoom-in");
  QPixmap pixZoomOut = LOAD_PIXMAP("zoom-out");
  QPixmap pixZoomReset = LOAD_PIXMAP("view-refresh");
  QIcon iconZoomIn(pixZoomIn);
  QIcon iconZoomOut(pixZoomOut);
  QIcon iconZoomReset(pixZoomReset);
  if (DialogSettings::darkThemeEnabled()) {
    iconZoomIn.addPixmap(darkerPixmap(pixZoomIn), QIcon::Disabled, QIcon::Off);
    iconZoomOut.addPixmap(darkerPixmap(pixZoomOut), QIcon::Disabled, QIcon::Off);
    iconZoomReset.addPixmap(darkerPixmap(pixZoomReset), QIcon::Disabled, QIcon::Off);
    ui->comboBox->setStyleSheet("QComboBox::disabled { background: rgb(40,40,40); } ");
  }
  ui->tbZoomIn->setIcon(iconZoomIn);
  ui->tbZoomOut->setIcon(iconZoomOut);
  ui->tbZoomReset->setIcon(iconZoomReset);

  connect(ui->comboBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onComboBoxEditingFinished()));
  connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboIndexChanged(int)));

  connect(ui->tbZoomIn, SIGNAL(clicked(bool)), this, SIGNAL(zoomIn()));
  connect(ui->tbZoomOut, SIGNAL(clicked(bool)), this, SIGNAL(zoomOut()));
  connect(ui->tbZoomReset, SIGNAL(clicked(bool)), this, SIGNAL(zoomReset()));
  setZoomConstraint(ZoomConstraint::Any);
}

ZoomLevelSelector::~ZoomLevelSelector()
{
  delete ui;
}

void ZoomLevelSelector::setZoomConstraint(ZoomConstraint constraint)
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
    text.remove(QRegExp(" ?%?$"));
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

QPixmap ZoomLevelSelector::darkerPixmap(const QPixmap & pixmap)
{
  QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
  for (int row = 0; row < image.height(); ++row) {
    QRgb * pixel = (QRgb *)image.scanLine(row);
    const QRgb * limit = pixel + image.width();
    while (pixel != limit) {
      QRgb grayed;
      if (qAlpha(*pixel) != 0) {
        grayed = qRgba((int)(0.4 * qRed(*pixel)), (int)(0.4 * qGreen(*pixel)), (int)(0.4 * qBlue(*pixel)), (int)(0.4 * qAlpha((*pixel))));
      } else {
        grayed = qRgba(0, 0, 0, 0);
      }
      *pixel++ = grayed;
    }
  }
  return QPixmap::fromImage(image);
}

ZoomLevelValidator::ZoomLevelValidator(QObject * parent) : QValidator(parent)
{
  _doubleValidator = new QDoubleValidator(1e-10, PREVIEW_MAX_ZOOM_FACTOR * 100.0, 3, parent);
  _doubleValidator->setNotation(QDoubleValidator::StandardNotation);
}

QValidator::State ZoomLevelValidator::validate(QString & input, int & pos) const
{
  QString str = input;
  str.remove(QRegExp(" ?%?$"));
  return _doubleValidator->validate(str, pos);
}
