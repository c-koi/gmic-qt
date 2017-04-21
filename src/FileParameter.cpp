/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FileParameter.cpp
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
#include "FileParameter.h"
#include "Common.h"
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QFileDialog>
#include <QPushButton>
#include <QFileInfo>
#include <QFontMetrics>
#include "DialogSettings.h"
#include "HtmlTranslator.h"

FileParameter::FileParameter(QObject *parent)
  : AbstractParameter(parent,true),
    _label(0),
    _button(0)
{
}

FileParameter::~FileParameter()
{
  delete _label;
  delete _button;
}

void
FileParameter::addTo(QWidget * widget, int row)
{
  QGridLayout * grid = dynamic_cast<QGridLayout*>(widget->layout());
  if (! grid) return;
  delete _label;
  delete _button;

  QString buttonText;
  if (_value.isEmpty()) {
    buttonText = "...";
  } else {
    int w = widget->contentsRect().width()/3;
    QFontMetrics fm(widget->font());
    buttonText = fm.elidedText(QFileInfo(_value).fileName(),Qt::ElideRight,w);
  }
  _button = new QPushButton(buttonText,widget);
  _button->setIcon(LOAD_ICON("document-open"));
  grid->addWidget(_label = new QLabel(_name,widget),row,0,1,1);
  grid->addWidget(_button,row,1,1,2);
  connect(_button, SIGNAL(clicked()),
          this, SLOT(onButtonPressed()));
}

QString
FileParameter::textValue() const
{
  return QString("\"%1\"").arg(_value);
}

QString
FileParameter::unquotedTextValue() const
{
  return _value;
}

void
FileParameter::setValue(const QString & value)
{
  _value = value;
  if (_button) {
    if (_value.isEmpty()) {
      _button->setText("...");
    } else {
      int width = _button->contentsRect().width()-10;
      QFontMetrics fm(_button->font());
      _button->setText(fm.elidedText(QFileInfo(_value).fileName(),Qt::ElideRight,width));
    }
  }
}

void
FileParameter::reset()
{
  _value = _default;
}

void FileParameter::initFromText(const char * text, int & textLength)
{
  QList<QString> list = parseText("file",text,textLength);
  _name = HtmlTranslator::html2txt(list[0]);
  _default = _value = list[1];
}

void
FileParameter::onButtonPressed()
{
  QString folder;
  if (!_value.isEmpty()) {
    folder = QFileInfo(_value).path();
  }
  QString filename = QFileDialog::getSaveFileName(0,tr("Select a file"),folder,QString(),0,
                                                  QFileDialog::DontConfirmOverwrite);
  if (filename.isNull()) {
    _value = "";
    _button->setText("...");
  } else {
    _value = filename;
    int w = _button->contentsRect().width()-10;
    QFontMetrics fm(_button->font());
    _button->setText(fm.elidedText(QFileInfo(_value).fileName(),Qt::ElideRight,w));
  }
  emit valueChanged();
}
