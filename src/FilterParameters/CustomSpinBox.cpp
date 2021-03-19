/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file CustomSpinBox.h
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
#include "FilterParameters/CustomSpinBox.h"
#include <QFontMetrics>
#include <QKeyEvent>
#include <QLineEdit>
#include <QShowEvent>
#include <QSizePolicy>
#include <QString>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include "Common.h"
#include "DialogSettings.h"

CustomSpinBox::CustomSpinBox(QWidget * parent, int min, int max) : QSpinBox(parent)
{
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  setRange(min, max);

  QSpinBox * dummy = new QSpinBox(this);
  dummy->hide();
  dummy->setRange(min, max);
  _sizeHint = dummy->sizeHint();
  _minimumSizeHint = dummy->minimumSizeHint();
  delete dummy;
  connect(this, &QSpinBox::editingFinished, [this]() { _unfinishedKeyboardEditing = false; });
}

CustomSpinBox::~CustomSpinBox() {}

QString CustomSpinBox::textFromValue(int value) const
{
  return QString::number(value);
}

QSize CustomSpinBox::sizeHint() const
{
  return _sizeHint;
}

QSize CustomSpinBox::minimumSizeHint() const
{
  return _minimumSizeHint;
}

void CustomSpinBox::keyPressEvent(QKeyEvent * event)
{
  QString text = event->text();
  if ((text.length() == 1 && text.front().isDigit()) || //
      (text == DialogSettings::NegativeSign) ||         //
      (text == DialogSettings::GroupSeparator) ||       //
      (event->key() == Qt::Key_Backspace) ||            //
      (event->key() == Qt::Key_Delete)) {
    _unfinishedKeyboardEditing = true;
  }
  QSpinBox::keyPressEvent(event);
}

int CustomSpinBox::integerPartDigitCount(float value)
{
  QString text = QString::number(static_cast<double>(value), 'f', 0);
  if (text[0] == QChar('-')) {
    text.remove(0, 1);
  }
  return text.length();
}
