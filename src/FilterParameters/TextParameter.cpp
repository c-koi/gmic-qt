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
#include "FilterTextTranslator.h"
#include "HtmlTranslator.h"
#include "IconLoader.h"

TextParameter::TextParameter(QObject * parent) : AbstractParameter(parent, true), _label(nullptr), _lineEdit(nullptr), _textEdit(nullptr), _multiline(false), _connected(false)
{
  _updateAction = nullptr;
}

TextParameter::~TextParameter()
{
  delete _lineEdit;
  delete _textEdit;
  delete _label;
}

bool TextParameter::addTo(QWidget * widget, int row)
{
  _grid = dynamic_cast<QGridLayout *>(widget->layout());
  Q_ASSERT_X(_grid, __PRETTY_FUNCTION__, "No grid layout in widget");
  _row = row;
  delete _label;
  delete _lineEdit;
  delete _textEdit;
  if (_multiline) {
    _label = nullptr;
    _lineEdit = nullptr;
    _textEdit = new MultilineTextParameterWidget(_name, _value, widget);
    _grid->addWidget(_textEdit, row, 0, 1, 3);
  } else {
    _grid->addWidget(_label = new QLabel(_name, widget), row, 0, 1, 1);
    _lineEdit = new QLineEdit(_value, widget);
    _textEdit = nullptr;
    _grid->addWidget(_lineEdit, row, 1, 1, 2);
#if QT_VERSION_GTE(5, 2, 0)
    _updateAction = _lineEdit->addAction(LOAD_ICON("view-refresh"), QLineEdit::TrailingPosition);
#endif
  }
  connectEditor();
  return true;
}

QString TextParameter::textValue() const
{
  QString text = _multiline ? _textEdit->text() : _lineEdit->text();
  text.replace(QChar('"'), QString("\\\""));
  return QString("\"%1\"").arg(text);
}

QString TextParameter::defaultTextValue() const
{
  QString text = _default;
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
  if (_textEdit) {
    disconnectEditor();
    _textEdit->setText(_value);
    connectEditor();
  } else if (_lineEdit) {
    disconnectEditor();
    _lineEdit->setText(_value);
    connectEditor();
  }
}

void TextParameter::reset()
{
  if (_textEdit) {
    _textEdit->setText(_default);
  } else if (_lineEdit) {
    _lineEdit->setText(_default);
  }
  _value = _default;
}

bool TextParameter::initFromText(const char * text, int & textLength)
{
  QStringList list = parseText("text", text, textLength);
  if (list.isEmpty()) {
    return false;
  }
  _name = HtmlTranslator::html2txt(FilterTextTranslator::translate(list[0]));
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

bool TextParameter::isQuoted() const
{
  return true;
}

void TextParameter::onValueChanged()
{
  notifyIfRelevant();
}

void TextParameter::connectEditor()
{
  if (_connected) {
    return;
  }
  if (_textEdit) {
    connect(_textEdit, SIGNAL(valueChanged()), this, SLOT(onValueChanged()));
  } else if (_lineEdit) {
    connect(_lineEdit, SIGNAL(editingFinished()), this, SLOT(onValueChanged()));
#if QT_VERSION_GTE(5, 2, 0)
    connect(_updateAction, SIGNAL(triggered(bool)), this, SLOT(onValueChanged()));
#endif
  }
  _connected = true;
}

void TextParameter::disconnectEditor()
{
  if (!_connected) {
    return;
  }
  if (_textEdit) {
    _textEdit->disconnect(this);
  } else if (_lineEdit) {
    _lineEdit->disconnect(this);
#if QT_VERSION_GTE(5, 2, 0)
    _updateAction->disconnect(this);
#endif
  }
  _connected = false;
}
