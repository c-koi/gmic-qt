/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterParametersWidget.cpp
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
#include "FilterParameters/PointParameter.h"
#include "Logger.h"
#include "Misc.h"

namespace GmicQt
{

FilterParametersWidget::FilterParametersWidget(QWidget * parent) : QWidget(parent), _valueString(""), _labelNoParams(nullptr), _paddingWidget(nullptr)
{
  delete layout();
  auto grid = new QGridLayout(this);
  grid->setRowStretch(1, 2);
  _labelNoParams = new QLabel(tr("<i>Select a filter</i>"), this);
  _labelNoParams->setAlignment(Qt::AlignHCenter | Qt::AlignCenter);
  grid->addWidget(_labelNoParams, 0, 0, 4, 3);
  _actualParametersCount = 0;
  _filterHash.clear();
  _hasKeypoints = false;
}

QVector<AbstractParameter *> FilterParametersWidget::buildParameters(const QString & filterName, //
                                                                     const QString & parameters, //
                                                                     QObject * parent,           //
                                                                     int * actualParameterCount, //
                                                                     QString * error)
{
  QVector<AbstractParameter *> result;
  QByteArray rawText = parameters.toUtf8();
  const char * cstr = rawText.constData();
  int length = 0;
  int localActualParameterCount = 0;
  QString localError;

  AbstractParameter * parameter;
  do {
    parameter = AbstractParameter::createFromText(filterName, cstr, length, localError, parent);
    if (parameter) {
      result.push_back(parameter);
      if (parameter->isActualParameter()) {
        localActualParameterCount += 1;
      }
    }
    cstr += length;
  } while (parameter && localError.isEmpty());

  if (!localError.isEmpty()) {
    for (AbstractParameter * p : result) {
      delete p;
    }
    result.clear();
    localError = QString("Parameter #%1\n%2").arg(localActualParameterCount + 1).arg(localError);
    localActualParameterCount = 0;
  }
  if (actualParameterCount) {
    *actualParameterCount = localActualParameterCount;
  }
  if (error) {
    *error = localError;
  }
  return result;
}

QStringList FilterParametersWidget::defaultParameterList(const QVector<AbstractParameter *> & parameters, //
                                                         QVector<bool> * quoted)
{
  if (quoted) {
    quoted->clear();
  }
  QStringList result;
  for (AbstractParameter * parameter : parameters) {
    if (parameter->isActualParameter()) {
      result.push_back(parameter->defaultValue());
      if (quoted) {
        quoted->push_back(parameter->isQuoted());
      }
    }
  }
  return result;
}

QStringList FilterParametersWidget::defaultParameterList(const QString & parametersDefinition, //
                                                         QString * error,                      //
                                                         QVector<bool> * quoted,               //
                                                         QVector<int> * size)
{
  if (error) {
    error->clear();
  }
  QObject parent;
  QString localError;
  QVector<AbstractParameter *> v = FilterParametersWidget::buildParameters("Dummy filter", parametersDefinition, &parent, nullptr, &localError);
  if (!localError.isEmpty()) {
    if (error) {
      *error = localError;
    }
    return QStringList();
  }
  QStringList result = defaultParameterList(v, quoted);
  if (size) {
    *size = parameterSizes(v);
  }
  return result;
}

QVector<bool> FilterParametersWidget::quotedParameters(const QVector<AbstractParameter *> & parameters)
{
  QVector<bool> result;
  for (AbstractParameter * p : parameters) {
    result.push_back(p->isQuoted());
  }
  return result;
}

QVector<int> FilterParametersWidget::parameterSizes(const QVector<AbstractParameter *> & parameters)
{
  QVector<int> result;
  for (AbstractParameter * p : parameters) {
    if (p->isActualParameter()) {
      result.push_back(p->size());
    }
  }
  return result;
}

bool FilterParametersWidget::build(const QString & name, const QString & hash, const QString & parameters, const QList<QString> & values, const QList<int> & visibilityStates)
{
  _filterName = name;
  _filterHash = hash;
  hide();
  clear();
  delete layout();
  auto grid = new QGridLayout(this);
  grid->setRowStretch(1, 2);

  PointParameter::resetDefaultColorIndex();

  // Build parameters and count actual ones
  QString error;
  _parameters = buildParameters(_filterName, parameters, this, &_actualParametersCount, &error);
  _quotedParameters = quotedParameters(_parameters);

  // Restore saved values
  if ((!values.isEmpty()) && (_actualParametersCount == values.size())) {
    QVector<AbstractParameter *>::iterator it = _parameters.begin();
    QList<QString>::const_iterator itValue = values.cbegin();
    while (it != _parameters.end()) {
      if ((*it)->isActualParameter()) {
        (*it)->setValue(*itValue);
        ++itValue;
      }
      ++it;
    }
  }

  // Add to widget and connect
  int row = 0;
  QVector<AbstractParameter *>::iterator it = _parameters.begin();
  while (it != _parameters.end()) {
    AbstractParameter * parameter = *it;
    if (parameter->addTo(this, row)) {
      parameter->hideWidgets();
      grid->setRowStretch(row, 0);
      ++row;
    }
    connect(parameter, &AbstractParameter::valueChanged, this, &FilterParametersWidget::updateValueStringAndNotify);
    ++it;
  }

  if (_actualParametersCount != visibilityStates.size()) {
    Logger::warning(QString("Parameters/SetVisibilities: Wrong number of values %1 (expecting %2)").arg(visibilityStates.size()).arg(_actualParametersCount));
  }

  if (visibilityStates.isEmpty() || (_actualParametersCount != visibilityStates.size())) {
    applyDefaultVisibilityStates();
  } else {
    setVisibilityStates(visibilityStates);
  }

  // Retrieve a dummy keypoint list
  KeypointList keypoints;
  it = _parameters.begin();
  while (it != _parameters.end()) {
    (*it)->addToKeypointList(keypoints);
    ++it;
  }
  _hasKeypoints = !keypoints.isEmpty();

  if (row > 0) {
    delete _labelNoParams;
    _labelNoParams = nullptr;
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
  show();
  return error.isEmpty();
}

void FilterParametersWidget::setNoFilter(const QString & message)
{
  clear();
  delete layout();
  auto grid = new QGridLayout(this);
  grid->setRowStretch(1, 2);

  if (message.isEmpty()) {
    _labelNoParams = new QLabel(tr("<i>Select a filter</i>"), this);
  } else {
    _labelNoParams = new QLabel(QString("<i>%1</i>").arg(message), this);
  }
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
  for (AbstractParameter * param : _parameters) {
    if (param->isActualParameter()) {
      list.append(param->value());
    }
  }
  return list;
}

void FilterParametersWidget::setValues(const QStringList & list, bool notify)
{
  if (list.isEmpty()) {
    return;
  }
  if (_actualParametersCount != list.size()) {
    Logger::warning(QString("Parameters/SetValues: Wrong number of values %1 (expecting %2)").arg(list.size()).arg(_actualParametersCount));
    return;
  }
  auto itValue = list.begin();
  for (AbstractParameter * param : _parameters) {
    if (param->isActualParameter()) {
      param->setValue(*itValue++);
    }
  }
  updateValueString(notify);
}

void FilterParametersWidget::setVisibilityStates(const QList<int> & states)
{
  if (states.isEmpty()) {
    return;
  }
  if (_actualParametersCount != states.size()) {
    Logger::warning(QString("Parameters/SetVisibilities: Wrong number of values %1 (expecting %2)").arg(states.size()).arg(_actualParametersCount));
    return;
  }

  // Fill a table of new states for all parameters, including no-value ones
  QVector<AbstractParameter::VisibilityState> newVisibilityStates(_parameters.size(), AbstractParameter::VisibilityState::Unspecified);
  {
    auto itState = states.begin();
    for (int n = 0; n < _parameters.size(); ++n) {
      AbstractParameter * parameter = _parameters[n];
      if (parameter->isActualParameter()) {
        newVisibilityStates[n] = static_cast<AbstractParameter::VisibilityState>(*itState);
        ++itState;
      }
    }
  }
  // Propagate if necessary
  for (int n = 0; n < _parameters.size(); ++n) {
    AbstractParameter * parameter = _parameters[n];
    if (parameter->isActualParameter()) {
      AbstractParameter::VisibilityState state = newVisibilityStates[n];
      if (state == AbstractParameter::VisibilityState::Unspecified) {
        state = parameter->defaultVisibilityState();
      }
      if ((parameter->visibilityPropagation() == AbstractParameter::VisibilityPropagation::Up) || //
          (parameter->visibilityPropagation() == AbstractParameter::VisibilityPropagation::UpDown)) {
        int i = n - 1;
        while ((i >= 0) && !_parameters[i]->isActualParameter()) {
          newVisibilityStates[i--] = state;
        }
      }
      if ((parameter->visibilityPropagation() == AbstractParameter::VisibilityPropagation::Down) || //
          (parameter->visibilityPropagation() == AbstractParameter::VisibilityPropagation::UpDown)) {
        int i = n + 1;
        while ((i < _parameters.size()) && !_parameters[i]->isActualParameter()) {
          newVisibilityStates[i++] = state;
        }
      }
    }
  }

  for (int n = 0; n < _parameters.size(); ++n) {
    AbstractParameter * const parameter = _parameters[n];
    parameter->setVisibilityState(newVisibilityStates[n]);
  }
}

QList<int> FilterParametersWidget::visibilityStates()
{
  QList<int> states;
  for (const AbstractParameter * const param : _parameters) {
    if (param->isActualParameter()) {
      states.push_back((int)param->visibilityState());
    }
  }
  return states;
}

QList<int> FilterParametersWidget::defaultVisibilityStates()
{
  QList<int> states;
  for (AbstractParameter * param : _parameters) {
    if (param->isActualParameter()) {
      states.push_back((int)param->defaultVisibilityState());
    }
  }
  return states;
}

void FilterParametersWidget::reset(bool notify)
{
  for (AbstractParameter * param : _parameters) {
    if (param->isActualParameter()) {
      param->reset();
    }
  }
  applyDefaultVisibilityStates();
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

QString FilterParametersWidget::valueString(const QVector<AbstractParameter *> & parameters)
{
  QString result;
  bool firstParameter = true;
  for (AbstractParameter * parameter : parameters) {
    if (parameter->isActualParameter()) {
      QString str = parameter->isQuoted() ? quotedString(parameter->value()) : parameter->value();
      if (!str.isNull()) {
        if (!firstParameter) {
          result += ",";
        }
        result += str;
        firstParameter = false;
      }
    }
  }
  return result;
}

QString FilterParametersWidget::defaultValueString(const QVector<AbstractParameter *> & parameters)
{
  QString result;
  bool firstParameter = true;
  for (AbstractParameter * parameter : parameters) {
    if (parameter->isActualParameter()) {
      QString str = parameter->isQuoted() ? quotedString(parameter->defaultValue()) : parameter->defaultValue();
      if (!str.isNull()) {
        if (!firstParameter) {
          result += ",";
        }
        result += str;
        firstParameter = false;
      }
    }
  }
  return result;
}

void FilterParametersWidget::updateValueString(bool notify)
{
  _valueString = valueString(_parameters);
  if (notify) {
    emit valueChanged();
  }
}

void FilterParametersWidget::updateValueStringAndNotify()
{
  updateValueString(true);
}

void FilterParametersWidget::clear()
{
  QVector<AbstractParameter *>::iterator it = _parameters.begin();
  while (it != _parameters.end()) {
    delete *it;
    ++it;
  }
  _parameters.clear();
  _actualParametersCount = 0;

  delete _labelNoParams;
  _labelNoParams = nullptr;

  delete _paddingWidget;
  _paddingWidget = nullptr;
}

void FilterParametersWidget::applyDefaultVisibilityStates()
{
  setVisibilityStates(defaultVisibilityStates()); // Will propagate
}

void FilterParametersWidget::clearButtonParameters()
{
  for (AbstractParameter * param : _parameters) {
    if (param->isActualParameter()) {
      param->clear();
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
  QVector<AbstractParameter *>::const_iterator it = _parameters.begin();
  while (it != _parameters.end()) {
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
  QVector<AbstractParameter *>::const_iterator it = _parameters.begin();
  while (it != _parameters.end()) {
    (*it)->extractPositionFromKeypointList(list);
    ++it;
  }
  updateValueString(notify);
}

bool FilterParametersWidget::hasKeypoints() const
{
  return _hasKeypoints;
}

const QVector<bool> & FilterParametersWidget::quotedParameters() const
{
  return _quotedParameters;
}

} // namespace GmicQt
