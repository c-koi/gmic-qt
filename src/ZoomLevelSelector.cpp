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
#include "ZoomLevelSelector.h"
#include <QDebug>
#include <QLineEdit>
#include <cmath>
#include "Common.h"
#include "ui_zoomlevelselector.h"

ZoomLevelSelector::ZoomLevelSelector(QWidget * parent) : QWidget(parent), ui(new Ui::ZoomLevelSelector)
{
  ui->setupUi(this);
  ui->comboBox->setEditable(true);
  QStringList values = {"1000 %", "800 %", "400 %", "200 %", "150 %", "100 %", "66.7 %", "50 %", "25 %", "12.5 %"};

  QString maxStr = values.front();
  maxStr.remove(" %");
  int max = maxStr.toInt();
  double previewMax = PREVIEW_MAX_ZOOM_FACTOR;
  while (max < previewMax * 100) {
    max += 1000;
    values.push_front(QString::number(max) + " %");
  }

  ui->comboBox->addItems(values);
  ui->comboBox->setInsertPolicy(QComboBox::NoInsert);
  ui->comboBox->setValidator(new ZoomLevelValidator(ui->comboBox));
  ui->comboBox->setCompleter(nullptr);
  _notificationsEnabled = true;

  connect(ui->comboBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onComboBoxEditingFinished()));
  connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboIndexChanged(int)));
}

ZoomLevelSelector::~ZoomLevelSelector()
{
  delete ui;
}

void ZoomLevelSelector::display(double zoom)
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
  _notificationsEnabled = false;
  ui->comboBox->setCurrentIndex(iMin);
  ui->comboBox->setEditText(text);
  _currentText = text;
  _notificationsEnabled = true;
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
  ui->comboBox->lineEdit()->setText(_currentText = text);
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
  str.remove(QRegExp(" ?%?$"));
  return _doubleValidator->validate(str, pos);
}
