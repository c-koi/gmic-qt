/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file SeparatorParameter.cpp
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
#include "FilterParameters/SeparatorParameter.h"
#include <QFrame>
#include <QGridLayout>
#include <QSizePolicy>
#include "Common.h"
#include "DialogSettings.h"

SeparatorParameter::SeparatorParameter(QObject * parent) : AbstractParameter(parent, false), _frame(nullptr) {}

SeparatorParameter::~SeparatorParameter()
{
  delete _frame;
}

bool SeparatorParameter::addTo(QWidget * widget, int row)
{
  _grid = dynamic_cast<QGridLayout *>(widget->layout());
  Q_ASSERT_X(_grid, __PRETTY_FUNCTION__, "No grid layout in widget");
  _row = row;
  delete _frame;
  _frame = new QFrame(widget);
  QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(_frame->sizePolicy().hasHeightForWidth());
  _frame->setSizePolicy(sizePolicy);
  _frame->setFrameShape(QFrame::HLine);
  _frame->setFrameShadow(QFrame::Sunken);
  if (DialogSettings::darkThemeEnabled()) {
    _frame->setStyleSheet("QFrame{ border-top: 0px none #a0a0a0; border-bottom: 2px solid rgb(160,160,160);}");
  }
  _grid->addWidget(_frame, row, 0, 1, 3);
  return true;
}

QString SeparatorParameter::textValue() const
{
  return QString();
}

void SeparatorParameter::setValue(const QString &) {}

void SeparatorParameter::reset() {}

bool SeparatorParameter::initFromText(const char * text, int & textLength)
{
  QStringList list = parseText("separator", text, textLength);
  unused(list);
  return true;
}
