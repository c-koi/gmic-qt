/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file CustomDoubleSpinBox.h
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
#include "FilterParameters/CustomDoubleSpinBox.h"
#include <QFontMetrics>
#include <QLineEdit>
#include <QShowEvent>
#include <QSizePolicy>
#include <QString>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include "Common.h"

CustomDoubleSpinBox::CustomDoubleSpinBox(QWidget * parent, float min, float max) : QDoubleSpinBox(parent)
{
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  const int decimals = std::max(2, MAX_DIGITS - std::max(integerPartDigitCount(min), integerPartDigitCount(max)));
  setDecimals(decimals);
  setRange(min, max);

  QDoubleSpinBox * dummy = new QDoubleSpinBox(this);
  dummy->hide();
  dummy->setRange(min, max);
  dummy->setDecimals(decimals);
  _sizeHint = dummy->sizeHint();
  _minimumSizeHint = dummy->minimumSizeHint();
  delete dummy;
}

CustomDoubleSpinBox::~CustomDoubleSpinBox() {}

QString CustomDoubleSpinBox::textFromValue(double value) const
{
  QString text = QString::number(value, 'g', MAX_DIGITS);
  if (text.contains('e') || text.contains('E')) {
    text = QString::number(value, 'f', decimals());
    const QChar DecimalPoint = QLocale().decimalPoint();
    if (text.contains(DecimalPoint)) {
      while (text.endsWith(QChar('0'))) {
        text.chop(1);
      }
      if (text.endsWith(DecimalPoint)) {
        text.chop(1);
      }
    }
  }
  return text;
}

QSize CustomDoubleSpinBox::sizeHint() const
{
  return _sizeHint;
}

QSize CustomDoubleSpinBox::minimumSizeHint() const
{
  return _minimumSizeHint;
}

int CustomDoubleSpinBox::integerPartDigitCount(float value)
{
  QString text = QString::number(static_cast<double>(value), 'f', 0);
  if (text[0] == QChar('-')) {
    text.remove(0, 1);
  }
  return text.length();
}
