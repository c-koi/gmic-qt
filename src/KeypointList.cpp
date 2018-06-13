/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file KeypointList.cpp
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

#include "KeypointList.h"
#include <cmath>
#include <cstring>

KeypointList::KeypointList() {}

void KeypointList::add(const KeypointList::Keypoint & keypoint)
{
  _keypoints.push_back(keypoint);
}

bool KeypointList::isEmpty()
{
  return _keypoints.empty();
}

void KeypointList::clear()
{
  _keypoints.clear();
}

QPointF KeypointList::position(int n) const
{
  const Keypoint & kp = _keypoints[n];
  return QPointF(kp.x, kp.y);
}

QColor KeypointList::color(int n) const
{
  return _keypoints[n].color;
}

bool KeypointList::isRemovable(int n) const
{
  return _keypoints[n].removable;
}

bool KeypointList::allowsBusrt(int n) const
{
  return _keypoints[n].burst;
}

KeypointList::Keypoint::Keypoint(float x, float y, QColor color, bool removable, bool burst, float radius) : x(x), y(y), color(color), removable(removable), burst(burst), radius(radius) {}

KeypointList::Keypoint::Keypoint(QPointF point, QColor color, bool removable, bool burst, float radius)
    : x((float)point.x()), y((float)point.y()), color(color), removable(removable), burst(burst), radius(radius)
{
}

KeypointList::Keypoint::Keypoint(QColor color, bool removable, bool burst, float radius) : color(color), removable(removable), burst(burst), radius(radius)
{
  setNaN();
}

bool KeypointList::Keypoint::isNaN() const
{
  if (sizeof(float) == 4) {
    unsigned int ix, iy;
    std::memcpy(&ix, &x, sizeof(float));
    std::memcpy(&iy, &y, sizeof(float));
    return ((ix & 0x7fffffff) > 0x7f800000) || ((iy & 0x7fffffff) > 0x7f800000);
  }
#ifdef isnan
  return (isnan(x) || isnan(y));
#else
  return !(x == x) || !(y == y);
#endif
}

KeypointList::Keypoint & KeypointList::Keypoint::setNaN()
{
#ifdef NAN
  x = y = (float)NAN;
#else
  const double nanValue = -std::sqrt(-1.0);
  x = y = (float)nanValue;
#endif
  return *this;
}
