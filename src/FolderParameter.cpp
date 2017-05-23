/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FolderParameter.cpp
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
#include "FolderParameter.h"
#include "Common.h"
#include <QWidget>
#include <QDebug>
#include <QRegExp>
#include <QGridLayout>
#include <QLabel>
#include <QFileDialog>
#include <QPushButton>
#include <QFileInfo>
#include <QFontMetrics>
#include "HtmlTranslator.h"
#include "DialogSettings.h"

FolderParameter::FolderParameter(QObject *parent)
  : AbstractParameter(parent,true),
    _label(0),
    _button(0)
{
}

FolderParameter::~FolderParameter()
{
  delete _label;
  delete _button;
}

void
FolderParameter::addTo(QWidget * widget, int row)
{
  QGridLayout * grid = dynamic_cast<QGridLayout*>(widget->layout());
  if (! grid) return;
  delete _label;
  delete _button;

  _button = new QPushButton(widget);
  _button->setIcon(LOAD_ICON("folder"));
  grid->addWidget(_label = new QLabel(_name,widget),row,0,1,1);
  grid->addWidget(_button,row,1,1,2);
  setValue(_value);
  connect(_button, SIGNAL(clicked()),
          this, SLOT(onButtonPressed()));
}

QString
FolderParameter::textValue() const
{
  return QString("\"%1\"").arg(_value);
}

QString
FolderParameter::unquotedTextValue() const
{
  return _value;
}

void
FolderParameter::setValue(const QString & value)
{
  _value = value;
  if ( _value.isEmpty() ) {
    _value = DialogSettings::FolderParameterDefaultValue;
  } else if ( ! QFileInfo(_value).isDir() ) {
    _value = QDir::homePath();
  }
  QDir dir(_value);
  QDir abs(dir.absolutePath());
  if (_button) {
    int width = _button->contentsRect().width()-10;
    QFontMetrics fm(_button->font());
    _button->setText(fm.elidedText(abs.dirName(),Qt::ElideRight,width));
  }
}

void
FolderParameter::reset()
{
  setValue(_default);
}

void FolderParameter::initFromText(const char * text, int & textLength)
{
  QList<QString> list = parseText("folder",text,textLength);
  _name = HtmlTranslator::html2txt(list[0]);
  QRegExp re("^\".*\"$");
  if ( re.exactMatch(list[1]) ) {
    list[1].chop(1);
    list[1].remove(0,1);
  }
  if ( list[1].isEmpty() ) {
    _default.clear();
    _value = DialogSettings::FolderParameterDefaultValue;
  } else {
    _default = _value = list[1];
  }
}

void
FolderParameter::onButtonPressed()
{
  QString oldValue = _value;
  QString path = QFileDialog::getExistingDirectory(0,tr("Select a folder"),_value);
  if (path.isEmpty()) {
    setValue(oldValue);
  } else {
    DialogSettings::FolderParameterDefaultValue = path;
    setValue(path);
  }
  emit valueChanged();
}
