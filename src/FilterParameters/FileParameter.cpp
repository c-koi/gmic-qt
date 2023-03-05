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
#include "FilterParameters/FileParameter.h"
#include <QApplication>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QWidget>
#include "FilterTextTranslator.h"
#include "HtmlTranslator.h"
#include "IconLoader.h"
#include "Settings.h"

namespace GmicQt
{

FileParameter::FileParameter(QObject * parent) : AbstractParameter(parent), _label(nullptr), _button(nullptr), _dialogMode(DialogMode::InputOutput) {}

FileParameter::~FileParameter()
{
  delete _label;
  delete _button;
}

int FileParameter::size() const
{
  return 1;
}

bool FileParameter::addTo(QWidget * widget, int row)
{
  _grid = dynamic_cast<QGridLayout *>(widget->layout());
  Q_ASSERT_X(_grid, __PRETTY_FUNCTION__, "No grid layout in widget");
  _row = row;
  delete _label;
  delete _button;

  QString buttonText;
  if (_value.isEmpty()) {
    buttonText = "...";
  } else {
    int w = widget->contentsRect().width() / 3;
    QFontMetrics fm(widget->font());
    buttonText = fm.elidedText(QFileInfo(_value).fileName(), Qt::ElideRight, w);
  }
  _button = new QPushButton(buttonText, widget);
  _button->setIcon(LOAD_ICON("document-open"));
  _grid->addWidget(_label = new QLabel(_name, widget), row, 0, 1, 1);
  setTextSelectable(_label);
  _grid->addWidget(_button, row, 1, 1, 2);
  connect(_button, &QPushButton::clicked, this, &FileParameter::onButtonPressed);
  return true;
}

QString FileParameter::value() const
{
  return _value;
}

QString FileParameter::defaultValue() const
{
  return _default;
}

void FileParameter::setValue(const QString & value)
{
  _value = value;
  if (_button) {
    if (_value.isEmpty()) {
      _button->setText("...");
    } else {
      int width = _button->contentsRect().width() - 10;
      QFontMetrics fm(_button->font());
      _button->setText(fm.elidedText(QFileInfo(_value).fileName(), Qt::ElideRight, width));
    }
  }
}

void FileParameter::reset()
{
  setValue(_default);
}

bool FileParameter::initFromText(const QString & filterName, const char * text, int & textLength)
{
  QList<QString> list;
  if (matchType("filein", text)) {
    list = parseText("filein", text, textLength);
    _dialogMode = DialogMode::Input;
  } else if (matchType("fileout", text)) {
    list = parseText("fileout", text, textLength);
    _dialogMode = DialogMode::Output;
  } else {
    list = parseText("file", text, textLength);
    _dialogMode = DialogMode::InputOutput;
  }
  if (list.isEmpty()) {
    return false;
  }
  _name = HtmlTranslator::html2txt(FilterTextTranslator::translate(list[0], filterName));
  QRegularExpression re("^\"(.*)\"$");
  QRegularExpressionMatch match = re.match(list[1]);
  if (match.hasMatch()) {
    list[1] = match.captured(1);
  }
  _default = _value = list[1];
  return true;
}

bool FileParameter::isQuoted() const
{
  return true;
}

void FileParameter::onButtonPressed()
{
  QString folder;
  if (_value.isEmpty()) {
    folder = Settings::FileParameterDefaultPath;
  } else {
    folder = QFileInfo(_value).path();
  }
  if (!QFileInfo(folder).isDir()) {
    folder = QDir::homePath();
  }

  QString filename;

  switch (_dialogMode) {
  case DialogMode::Input:
    filename = QFileDialog::getOpenFileName(QApplication::topLevelWidgets().at(0), tr("Select a file"), folder, QString(), nullptr);
    break;
  case DialogMode::Output:
    filename = QFileDialog::getSaveFileName(QApplication::topLevelWidgets().at(0), tr("Select a file"), folder, QString(), nullptr);
    break;
  case DialogMode::InputOutput: {
    QFileDialog dialog(dynamic_cast<QWidget *>(parent()), tr("Select a file"), folder, QString());
    dialog.setOptions(QFileDialog::DontConfirmOverwrite | QFileDialog::DontUseNativeDialog);
    dialog.setFileMode(QFileDialog::AnyFile);
    if (!_value.isEmpty()) {
      dialog.selectFile(_value);
    }
    dialog.exec();
    QStringList filenames = dialog.selectedFiles();
    if (!filenames.isEmpty() && !QFileInfo(filenames.front()).isDir()) {
      filename = filenames.front();
    }
  } break;
  }

  if (filename.isEmpty()) {
    _value.clear();
    _button->setText("...");
  } else {
    _value = filename;
    QFileInfo info(filename);
    Settings::FileParameterDefaultPath = info.path();
    int w = _button->contentsRect().width() - 10;
    QFontMetrics fm(_button->font());
    _button->setText(fm.elidedText(QFileInfo(_value).fileName(), Qt::ElideRight, w));
  }
  notifyIfRelevant();
}

} // namespace GmicQt
