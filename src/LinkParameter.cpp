/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file LinkParameter.cpp
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
#include "LinkParameter.h"
#include "Common.h"
#include <QString>
#include <QUrl>
#include <QLabel>
#include <QGridLayout>
#include <QDesktopServices>
#include <QDebug>
#include "HtmlTranslator.h"

LinkParameter::LinkParameter(QObject *parent)
  : AbstractParameter(parent,false),
    _label(0),
    _alignment(Qt::AlignLeft)
{
}

LinkParameter::~LinkParameter()
{
  delete _label;
}

void
LinkParameter::addTo(QWidget * widget, int row)
{
  QGridLayout * grid = dynamic_cast<QGridLayout*>(widget->layout());
  if (! grid) return;
  delete _label;
  _label = new QLabel(QString("<a href=\"%2\">%1</a>").arg(_text).arg(_url),widget);
  _label->setAlignment(_alignment);
  _label->setTextFormat(Qt::RichText);
  _label->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
  connect(_label, SIGNAL(linkActivated(QString)),
          this, SLOT(onLinkActivated(QString)));
  grid->addWidget(_label,row,0,1,3);
}

QString
LinkParameter::textValue() const
{
  return QString::null;
}

void
LinkParameter::setValue(const QString &)
{
}

void
LinkParameter::reset()
{
}

void
LinkParameter::initFromText(const char * text, int & textLength)
{
  QList<QString> list = parseText("[lL]ink",text,textLength);
  QList<QString> values = list[1].split(QChar(','));

  if ( values.size() == 3 ) {
    float a = values[0].toFloat();
    if ( a == 0.0f ) {
      _alignment = Qt::AlignLeft;
    } else if ( a == 1.0f ) {
      _alignment = Qt::AlignRight;
    } else {
      _alignment = Qt::AlignCenter;
    }
    values.pop_front();
  } else {
    _alignment = Qt::AlignCenter;
  }

  if ( values.size() == 2 ) {
    _text = values[0].trimmed().remove(QRegExp("^\"")).remove(QRegExp("\"$"));
    _text = HtmlTranslator::html2txt(_text);
    values.pop_front();
  }
  if ( values.size() == 1 ) {
    _url = values[0].trimmed().remove(QRegExp("^\"")).remove(QRegExp("\"$"));
  }
  if ( _text.isEmpty() ) {
    _text = _url;
  }
}

void
LinkParameter::onLinkActivated(const QString &link)
{
  QDesktopServices::openUrl(QUrl(link));
}
