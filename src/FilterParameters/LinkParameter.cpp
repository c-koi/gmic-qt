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
#include "FilterParameters/LinkParameter.h"
#include <QDebug>
#include <QDesktopServices>
#include <QGridLayout>
#include <QLabel>
#include <QString>
#include <QUrl>
#include "Common.h"
#include "HtmlTranslator.h"

LinkParameter::LinkParameter(QObject * parent) : AbstractParameter(parent, false), _label(nullptr), _alignment(Qt::AlignLeft) {}

LinkParameter::~LinkParameter()
{
  delete _label;
}

bool LinkParameter::addTo(QWidget * widget, int row)
{
  _grid = dynamic_cast<QGridLayout *>(widget->layout());
  Q_ASSERT_X(_grid, __PRETTY_FUNCTION__, "No grid layout in widget");
  _row = row;
  delete _label;
  _label = new QLabel(QString("<a href=\"%2\">%1</a>").arg(_text).arg(_url), widget);
  _label->setAlignment(_alignment);
  _label->setTextFormat(Qt::RichText);
  _label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  connect(_label, SIGNAL(linkActivated(QString)), this, SLOT(onLinkActivated(QString)));
  _grid->addWidget(_label, row, 0, 1, 3);
  return true;
}

QString LinkParameter::textValue() const
{
  return QString::null;
}

void LinkParameter::setValue(const QString &) {}

void LinkParameter::reset() {}

bool LinkParameter::initFromText(const char * text, int & textLength)
{
  QList<QString> list = parseText("link", text, textLength);
  QList<QString> values = list[1].split(QChar(','));

  if (values.size() == 3) {
    bool ok;
    float a = values[0].toFloat(&ok);
    if (!ok) {
      return false;
    }
    if (a == 0.0f) {
      _alignment = Qt::AlignLeft;
    } else if (a == 1.0f) {
      _alignment = Qt::AlignRight;
    } else {
      _alignment = Qt::AlignCenter;
    }
    values.pop_front();
  } else {
    _alignment = Qt::AlignCenter;
  }

  if (values.size() == 2) {
    _text = values[0].trimmed().remove(QRegExp("^\"")).remove(QRegExp("\"$"));
    _text = HtmlTranslator::html2txt(_text);
    values.pop_front();
  }
  if (values.size() == 1) {
    _url = values[0].trimmed().remove(QRegExp("^\"")).remove(QRegExp("\"$"));
  }
  if (values.isEmpty()) {
    return false;
  }
  if (_text.isEmpty()) {
    _text = _url;
  }
  return true;
}

void LinkParameter::onLinkActivated(const QString & link)
{
  QDesktopServices::openUrl(QUrl(link));
}
