/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterParamsWidget.cpp
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
#include "Common.h"
#include "FilterParamsWidget.h"
#include "AbstractParameter.h"
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QDebug>
#include <ParametersCache.h>
#include "FiltersTreeAbstractFilterItem.h"
#include "FiltersTreeFaveItem.h"

FilterParamsWidget::FilterParamsWidget(QWidget * parent)
  : QWidget(parent),
    _valueString(""),
    _labelNoParams(0),
    _paddingWidget(0)
{
  delete layout();
  QGridLayout * grid = new QGridLayout(this);
  grid->setRowStretch(1,2);
  _labelNoParams = new QLabel(tr("<i>Select a filter</i>"),this);
  _labelNoParams->setAlignment(Qt::AlignHCenter|Qt::AlignCenter);
  grid->addWidget(_labelNoParams,0,0,4,3);
  _actualParametersCount = 0;
  _filterHash.clear();
}

void
FilterParamsWidget::build(const FiltersTreeAbstractFilterItem *item, const QList<QString> & values)
{
  _filterName = item->name();
  _command = item->command();
  _previewCommand = item->previewCommand();
  _filterHash = item->hash();
  clear();
  delete layout();
  QGridLayout * grid = new QGridLayout(this);
  grid->setRowStretch(1,2);

  QList<QString> savedValues;

  if ( values.isEmpty() ) {
    savedValues = ParametersCache::getValues(item->hash());
    if ( savedValues.isEmpty() ) {
      const FiltersTreeFaveItem * fave = dynamic_cast<const FiltersTreeFaveItem*>(item);
      if ( fave ) {
        savedValues = fave->defaultValues();
      }
    }
  } else {
    savedValues = values;
  }

  AbstractParameter * parameter;
  QByteArray rawText = item->parameters().toLatin1();
  const char * cstr = rawText.constData();
  int length;

  // Build parameters and count actual ones
  _actualParametersCount = 0;
  QString error;
  do {
    parameter = AbstractParameter::createFromText(cstr,length,error,this);
    if (parameter) {
      _presetParameters.push_back(parameter);
      if ( parameter->isActualParameter()) {
        _actualParametersCount += 1;
      }
    }
    cstr += length;
  } while (parameter && error.isEmpty());

  if ( !error.isEmpty() ) {
    for ( AbstractParameter * p : _presetParameters) {
      delete p;
    }
    _presetParameters.clear();
    _actualParametersCount = 0;
    _command = "skip";
    _previewCommand = "skip";
  }

  // Restore saved values
  if ( savedValues.size() && (_actualParametersCount == savedValues.size()) ) {
    QVector<AbstractParameter*>::iterator it = _presetParameters.begin();
    while (it != _presetParameters.end()) {
      if ((*it)->isActualParameter() ) {
        (*it)->setValue(savedValues.front());
        savedValues.pop_front();
      }
      ++it;
    }
  }

  // Add to widget and connect
  int row = 0;
  QVector<AbstractParameter*>::iterator it = _presetParameters.begin();
  while (it != _presetParameters.end()) {
    if ((*it)->isVisible() ) {
      (*it)->addTo(this,row++);
      grid->setRowStretch(row-1,0);
    }
    connect( *it, SIGNAL(valueChanged()),
             this, SLOT(updateValueString()));
    ++it;
  }

  if (row > 0) {
    delete _labelNoParams;
    _labelNoParams = 0;
    _paddingWidget = new QWidget(this);
    _paddingWidget->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
    grid->addWidget(_paddingWidget,row++,0,1,3);
    grid->setRowStretch(row-1,1);
  } else {
    if ( error.isEmpty() ) {
      _labelNoParams = new QLabel(tr("<i>No parameters</i>"),this);
      _labelNoParams->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    } else {
      _labelNoParams = new QLabel(tr("<i>Error parsing filter parameters</i>").arg(error),this);
      QString text = error;
      if ( text.size() > 250 ) {
        text.truncate(250);
        text += "...";
      }
      _labelNoParams->setToolTip(text);
      _labelNoParams->setWordWrap(true);
      _labelNoParams->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    }
    grid->addWidget(_labelNoParams,0,0,4,3);
  }
  updateValueString(false);
}

void FilterParamsWidget::setNoFilter()
{
  clear();
  delete layout();
  QGridLayout * grid = new QGridLayout(this);
  grid->setRowStretch(1,2);

  _labelNoParams = new QLabel(tr("<i>Select a filter</i>"),this);
  _labelNoParams->setAlignment(Qt::AlignHCenter|Qt::AlignCenter);
  grid->addWidget(_labelNoParams,0,0,4,3);

  _valueString.clear();
  _command.clear();
  _previewCommand.clear();
  _filterHash.clear();
}

FilterParamsWidget::~FilterParamsWidget()
{
  clear();
}

const QString &
FilterParamsWidget::valueString() const
{
  return _valueString;
}

QStringList
FilterParamsWidget::valueStringList() const
{
  QStringList list;
  for (int i = 0; i < _presetParameters.size(); ++i) {
    if ( _presetParameters[i]->isActualParameter() ) {
      list.append(_presetParameters[i]->unquotedTextValue());
    }
  }
  return list;
}

void
FilterParamsWidget::setValues(const QStringList & list, bool notify)
{
  if (_actualParametersCount != list.size()) {
    return;
  }
  for (int i = 0, j = 0; i < _presetParameters.size(); ++i) {
    if ( _presetParameters[i]->isActualParameter() ) {
      _presetParameters[i]->setValue(list[j++]);
    }
  }
  updateValueString(notify);
}

void
FilterParamsWidget::reset(bool notify)
{
  for (int i = 0; i < _presetParameters.size(); ++i) {
    if ( _presetParameters[i]->isActualParameter() ) {
      _presetParameters[i]->reset();
    }
  }
  updateValueString(notify);
}

QString FilterParamsWidget::command() const
{
  return _command;
}

QString FilterParamsWidget::previewCommand() const
{
  return _previewCommand;
}

QString
FilterParamsWidget::filterName() const
{
  return _filterName;
}

int
FilterParamsWidget::actualParametersCount() const
{
  return _actualParametersCount;
}

QString FilterParamsWidget::filterHash() const
{
  return _filterHash;
}

void
FilterParamsWidget::updateValueString(bool notify)
{
  _valueString.clear();
  bool firstParameter = true;
  for (int i = 0; i < _presetParameters.size(); ++i) {
    if ( _presetParameters[i]->isActualParameter() ) {
      QString str = _presetParameters[i]->textValue();
      if (!str.isNull()) {
        if (!firstParameter) {
          _valueString += ",";
        }
        _valueString += str;
        firstParameter = false;
      }
    }
  }
  if (notify) {
    emit valueChanged();
  }
}

void
FilterParamsWidget::clear()
{
  QVector<AbstractParameter*>::iterator it = _presetParameters.begin();
  while (it != _presetParameters.end()) {
    delete *it;
    ++it;
  }
  _presetParameters.clear();
  _actualParametersCount = 0;

  delete _labelNoParams;
  _labelNoParams = 0;

  delete _paddingWidget;
  _paddingWidget = 0;
}
