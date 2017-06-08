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
#include "SeparatorParameter.h"
#include "Common.h"
#include <QFrame>
#include <QSizePolicy>
#include <QGridLayout>

SeparatorParameter::SeparatorParameter(QObject *parent)
  : AbstractParameter(parent,false),
    _frame(0)
{
}

SeparatorParameter::~SeparatorParameter()
{
  delete _frame;
}

void
SeparatorParameter::addTo(QWidget * widget, int row)
{
  QGridLayout * grid = dynamic_cast<QGridLayout*>(widget->layout());
  if (! grid) return;
  delete _frame;
  _frame = new QFrame(widget);
  QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(_frame->sizePolicy().hasHeightForWidth());
  _frame->setSizePolicy(sizePolicy);
  _frame->setFrameShape(QFrame::HLine);
  _frame->setFrameShadow(QFrame::Sunken);
  grid->addWidget(_frame,row,0,1,3);
}

QString
SeparatorParameter::textValue() const
{
  return QString::null;
}

void SeparatorParameter::setValue(const QString &)
{
}

void
SeparatorParameter::reset()
{
}

void SeparatorParameter::initFromText(const char * text, int & textLength)
{
  QStringList list = parseText("separator",text,textLength);
  unused(list);
}
