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
#include "FilterParameters/FilterParametersWidget.h"
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include "Common.h"
#include "FilterParameters/AbstractParameter.h"

FilterParametersWidget::FilterParametersWidget(QWidget * parent) : QWidget(parent), _valueString(""), _labelNoParams(0), _paddingWidget(0)
{
  delete layout();
  QGridLayout * grid = new QGridLayout(this);
  grid->setRowStretch(1, 2);
  _labelNoParams = new QLabel(tr("<i>Select a filter</i>"), this);
  _labelNoParams->setAlignment(Qt::AlignHCenter | Qt::AlignCenter);
  grid->addWidget(_labelNoParams, 0, 0, 4, 3);
  _actualParametersCount = 0;
  _filterHash.clear();
  _hasKeypoints = false;
}

bool FilterParametersWidget::build(const QString & name, const QString & hash, const QString & parameters, const QList<QString> & values)
{
  _filterName = name;
  _filterHash = hash;
  clear();
  delete layout();
  QGridLayout * grid = new QGridLayout(this);
  grid->setRowStretch(1, 2);

  QByteArray rawText = parameters.toLatin1();
  const char * cstr = rawText.constData();
  int length;

  // Build parameters and count actual ones
  _actualParametersCount = 0;
  QString error;
  AbstractParameter * parameter;
  do {
    parameter = AbstractParameter::createFromText(cstr, length, error, this);
    if (parameter) {
      _presetParameters.push_back(parameter);
      if (parameter->isActualParameter()) {
        _actualParametersCount += 1;
      }
    }
    cstr += length;
  } while (parameter && error.isEmpty());

  if (!error.isEmpty()) {
    for (AbstractParameter * p : _presetParameters) {
      delete p;
    }
    _presetParameters.clear();
    error = QString("Parameter #%1\n%2").arg(_actualParametersCount + 1).arg(error);
    _actualParametersCount = 0;
  }

  // Restore saved values
  if (values.size() && (_actualParametersCount == values.size())) {
    QVector<AbstractParameter *>::iterator it = _presetParameters.begin();
    QList<QString>::const_iterator itValue = values.cbegin();
    while (it != _presetParameters.end()) {
      if ((*it)->isActualParameter()) {
        (*it)->setValue(*itValue);
        ++itValue;
      }
      ++it;
    }
  }

  // Add to widget and connect
  int row = 0;
  QVector<AbstractParameter *>::iterator it = _presetParameters.begin();
  while (it != _presetParameters.end()) {
    if ((*it)->isVisible()) {
      (*it)->addTo(this, row++);
      grid->setRowStretch(row - 1, 0);
    }
    connect(*it, SIGNAL(valueChanged()), this, SLOT(updateValueString()));
    ++it;
  }

  // Retrieve a dummy keypoint list
  KeypointList keypoints;
  it = _presetParameters.begin();
  while (it != _presetParameters.end()) {
    (*it)->addToKeypointList(keypoints);
    ++it;
  }
  _hasKeypoints = !keypoints.isEmpty();

  if (row > 0) {
    delete _labelNoParams;
    _labelNoParams = 0;
    _paddingWidget = new QWidget(this);
    _paddingWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    grid->addWidget(_paddingWidget, row++, 0, 1, 3);
    grid->setRowStretch(row - 1, 1);
  } else {
    if (error.isEmpty()) {
      _labelNoParams = new QLabel(tr("<i>No parameters</i>"), this);
      _labelNoParams->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      _labelNoParams->setTextFormat(Qt::RichText);
    } else {
      QString errorMessage;
      errorMessage += tr("Error parsing filter parameters\n\n");
      QString text = error;
      if (text.size() > 250) {
        text.truncate(250);
        text += "...";
      }
      errorMessage += text;
      _labelNoParams = new QLabel(errorMessage, this);
      _labelNoParams->setToolTip(text);
      _labelNoParams->setWordWrap(true);
      _labelNoParams->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      _labelNoParams->setTextFormat(Qt::PlainText);
    }
    grid->addWidget(_labelNoParams, 0, 0, 4, 3);
  }
  updateValueString(false);
  return error.isEmpty();
}

void FilterParametersWidget::setNoFilter()
{
  clear();
  delete layout();
  QGridLayout * grid = new QGridLayout(this);
  grid->setRowStretch(1, 2);

  _labelNoParams = new QLabel(tr("<i>Select a filter</i>"), this);
  _labelNoParams->setAlignment(Qt::AlignHCenter | Qt::AlignCenter);
  grid->addWidget(_labelNoParams, 0, 0, 4, 3);

  _valueString.clear();
  _filterHash.clear();
}

FilterParametersWidget::~FilterParametersWidget()
{
  clear();
}

const QString & FilterParametersWidget::valueString() const
{
  return _valueString;
}

QStringList FilterParametersWidget::valueStringList() const
{
  QStringList list;
  for (int i = 0; i < _presetParameters.size(); ++i) {
    if (_presetParameters[i]->isActualParameter()) {
      list.append(_presetParameters[i]->unquotedTextValue());
    }
  }
  return list;
}

void FilterParametersWidget::setValues(const QStringList & list, bool notify)
{
  if (list.isEmpty() || _actualParametersCount != list.size()) {
    return;
  }
  for (int i = 0, j = 0; i < _presetParameters.size(); ++i) {
    if (_presetParameters[i]->isActualParameter()) {
      _presetParameters[i]->setValue(list[j++]);
    }
  }
  updateValueString(notify);
}

void FilterParametersWidget::reset(bool notify)
{
  for (int i = 0; i < _presetParameters.size(); ++i) {
    if (_presetParameters[i]->isActualParameter()) {
      _presetParameters[i]->reset();
    }
  }
  updateValueString(notify);
}

QString FilterParametersWidget::filterName() const
{
  return _filterName;
}

int FilterParametersWidget::actualParametersCount() const
{
  return _actualParametersCount;
}

QString FilterParametersWidget::filterHash() const
{
  return _filterHash;
}

void FilterParametersWidget::updateValueString(bool notify)
{
  _valueString.clear();
  bool firstParameter = true;
  for (int i = 0; i < _presetParameters.size(); ++i) {
    if (_presetParameters[i]->isActualParameter()) {
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

void FilterParametersWidget::clear()
{
  QVector<AbstractParameter *>::iterator it = _presetParameters.begin();
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

void FilterParametersWidget::clearButtonParameters()
{
  for (int i = 0; i < _presetParameters.size(); ++i) {
    if (_presetParameters[i]->isActualParameter()) {
      _presetParameters[i]->clear();
    }
  }
  updateValueString(false);
}

KeypointList FilterParametersWidget::keypoints() const
{
  KeypointList list;
  if (!_hasKeypoints) {
    return list;
  }
  QVector<AbstractParameter *>::const_iterator it = _presetParameters.begin();
  while (it != _presetParameters.end()) {
    (*it)->addToKeypointList(list);
    ++it;
  }
  return list;
}

void FilterParametersWidget::setKeypoints(KeypointList list, bool notify)
{
  Q_ASSERT_X((list.isEmpty() || _hasKeypoints), __PRETTY_FUNCTION__, "Keypoint list mismatch");
  if (!_hasKeypoints) {
    return;
  }
  QVector<AbstractParameter *>::const_iterator it = _presetParameters.begin();
  while (it != _presetParameters.end()) {
    (*it)->extractPositionFromKeypointList(list);
    ++it;
  }
  updateValueString(notify);
}

bool FilterParametersWidget::hasKeypoints() const
{
  return _hasKeypoints;
}
