/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file TextParameter.cpp
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
#include "FilterParameters/TextParameter.h"
#include <QAction>
#include <QDebug>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QWidget>
#include "Common.h"
#include "DialogSettings.h"
#include "FilterParameters/MultilineTextParameterWidget.h"
#include "HtmlTranslator.h"

TextParameter::TextParameter(QObject * parent) : AbstractParameter(parent, true), _label(0), _lineEdit(0), _textEdit(0), _multiline(false)
{
  _updateAction = 0;
}

TextParameter::~TextParameter()
{
  delete _lineEdit;
  delete _textEdit;
  delete _label;
}

void TextParameter::addTo(QWidget * widget, int row)
{
  QGridLayout * grid = dynamic_cast<QGridLayout *>(widget->layout());
  if (!grid)
    return;
  delete _label;
  delete _lineEdit;
  delete _textEdit;
  if (_multiline) {
    _label = 0;
    _lineEdit = 0;
    _textEdit = new MultilineTextParameterWidget(_name, _value, widget);
    grid->addWidget(_textEdit, row, 0, 1, 3);
    connect(_textEdit, SIGNAL(valueChanged()), this, SLOT(onValueChanged()));
  } else {
    grid->addWidget(_label = new QLabel(_name, widget), row, 0, 1, 1);
    _lineEdit = new QLineEdit(_value, widget);
    _textEdit = 0;
    grid->addWidget(_lineEdit, row, 1, 1, 2);
    connect(_lineEdit, SIGNAL(editingFinished()), this, SLOT(onValueChanged()));
#if QT_VERSION >= 0x050200
    _updateAction = _lineEdit->addAction(LOAD_ICON("view-refresh"), QLineEdit::TrailingPosition);
    connect(_updateAction, SIGNAL(triggered(bool)), this, SLOT(onValueChanged()));
#endif
  }
}

QString TextParameter::textValue() const
{
  QString text = _multiline ? _textEdit->text() : _lineEdit->text();
  text.replace(QChar('"'), QString("\\\""));
  return QString("\"%1\"").arg(text);
}

QString TextParameter::unquotedTextValue() const
{
  return _multiline ? _textEdit->text() : _lineEdit->text();
}

void TextParameter::setValue(const QString & value)
{
  _value = value;
  if (_multiline && _textEdit) {
    _textEdit->setText(_value);
  } else if (_lineEdit) {
    _lineEdit->setText(_value);
  }
}

void TextParameter::reset()
{
  if (_multiline) {
    _textEdit->setText(_default);
  } else {
    _lineEdit->setText(_default);
  }
  _value = _default;
}

bool TextParameter::initFromText(const char * text, int & textLength)
{
  QStringList list = parseText("text", text, textLength);
  _name = HtmlTranslator::html2txt(list[0]);
  QString value = list[1];
  _multiline = false;
  QRegExp re("^\\s*(0|1)\\s*,");
  if (value.contains(re) && re.matchedLength() > 0) {
    _multiline = (re.cap(1).toInt() == 1);
    value.replace(re, "");
  }
  value = value.trimmed().remove(QRegExp("^\"")).remove(QRegExp("\"$"));
  value.replace(QString("\\\\"), QString("\\"));
  value.replace(QString("\\n"), QString("\n"));
  _value = _default = value;
  return true;
}

void TextParameter::onValueChanged()
{
  notifyIfRelevant();
}
