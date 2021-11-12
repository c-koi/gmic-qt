/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file Tags.h
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
#ifndef GMIC_QT_TAGS_H
#define GMIC_QT_TAGS_H

#include <QColor>
#include <QDebug>
#include <QIcon>
#include <QString>
#include <QVector>
#include "Common.h"
class QAction;
class QObject;

namespace GmicQt
{

enum class TagColor
{
  None = 0,
  Red,
  Green,
  Blue,
  Cyan,
  Magenta,
  Yellow,
  Count
};

class TagAssets {
public:
  enum class IconMark
  {
    None,
    Check,
    Disk
  };
  static const QString & markerHtml(TagColor color, unsigned int sideSize);
  static const QIcon & menuIcon(TagColor color, IconMark mark);
  static QAction * action(QObject * parent, TagColor color, IconMark mark);
  static QColor colors[static_cast<unsigned int>(TagColor::Count)];
  static QString colorName(TagColor color);

private:
  static QString _markerHtml[static_cast<unsigned int>(TagColor::Count)];
  static QIcon _menuIcons[static_cast<unsigned int>(TagColor::Count)];
  static QIcon _menuIconsWithCheck[static_cast<unsigned int>(TagColor::Count)];
  static QIcon _menuIconsWithDisk[static_cast<unsigned int>(TagColor::Count)];
  static unsigned int _markerSideSize[static_cast<unsigned int>(TagColor::Count)];
};

class TagColorSet {
public:
  inline void toggle(TagColor color);
  inline void insert(TagColor color);
  inline bool contains(TagColor color) const;
  inline bool isEmpty() const;
  inline void clear();
  inline bool operator==(const TagColorSet & other) const;
  inline bool operator!=(const TagColorSet & other) const;
  inline TagColorSet operator+(TagColor color) const;
  inline TagColorSet & operator+=(TagColor color);
  inline TagColorSet & operator-=(TagColor color);
  inline TagColorSet operator-(TagColor color) const;
  inline TagColorSet operator|(const TagColorSet & other) const;
  inline TagColorSet & operator|=(const TagColorSet & other);
  inline TagColorSet operator&(const TagColorSet & other) const;
  inline TagColorSet & operator&=(const TagColorSet & other);
  inline unsigned int mask() const;
  static const TagColorSet Full;
  static const TagColorSet ActualColors;
  static const TagColorSet Empty;
  friend class const_iterator;
  class const_iterator {
  public:
    inline const_iterator(const TagColorSet & set);
    inline bool isAtEnd() const;
    inline const_iterator & operator++();
    inline const_iterator operator++(int);
    inline TagColor operator*() const;
    inline bool operator!=(const const_iterator & other) const;
    inline bool operator==(const const_iterator & other) const;

  private:
    int _position;
    const TagColorSet & _set;
  };
  inline const_iterator begin() const;
  inline const_iterator end() const;

  TagColorSet() : _mask(0u) {}
  TagColorSet(const TagColorSet & other) : _mask(other._mask) {}
  TagColorSet & operator=(const TagColorSet & other);
  inline explicit TagColorSet(unsigned int mask);

private:
  unsigned int _mask;
  static const unsigned int _fullMask = ((1 << int(TagColor::Count)) - 1);
};

std::ostream & operator<<(std::ostream & out, const TagColorSet & colors);

TagColorSet::TagColorSet(unsigned int mask) : _mask(mask & _fullMask) {}

inline void TagColorSet::toggle(TagColor color)
{
  Q_ASSERT_X((color != TagColor::Count), __PRETTY_FUNCTION__, QString("Inavild color (%1)").arg(int(color)).toLocal8Bit().constData());
  _mask ^= (1u << int(color));
}

void TagColorSet::insert(TagColor color)
{
  Q_ASSERT_X((color != TagColor::Count), __PRETTY_FUNCTION__, QString("Inavild color (%1)").arg(int(color)).toLocal8Bit().constData());
  _mask |= (1u << int(color));
}

bool TagColorSet::contains(TagColor color) const
{
  Q_ASSERT_X((color != TagColor::Count), __PRETTY_FUNCTION__, QString("Inavild color (%1)").arg(int(color)).toLocal8Bit().constData());
  return _mask & (1u << int(color));
}

bool TagColorSet::isEmpty() const
{
  return !_mask;
}

void TagColorSet::clear()
{
  _mask = 0;
}

bool TagColorSet::operator==(const TagColorSet & other) const
{
  return _mask == other._mask;
}

bool TagColorSet::operator!=(const TagColorSet & other) const
{
  return _mask != other._mask;
}

TagColorSet & TagColorSet::operator+=(TagColor color)
{
  insert(color);
  return *this;
}

TagColorSet TagColorSet::operator+(TagColor color) const
{
  TagColorSet result(*this);
  result.insert(color);
  return result;
}

TagColorSet & TagColorSet::operator-=(TagColor color)
{
  _mask &= ~(1u << int(color));
  return *this;
}

TagColorSet TagColorSet::operator-(TagColor color) const
{
  TagColorSet result(*this);
  result -= color;
  return result;
}

TagColorSet TagColorSet::operator|(const TagColorSet & other) const
{
  return TagColorSet(_mask | other._mask);
}

TagColorSet & TagColorSet::operator|=(const TagColorSet & other)
{
  _mask |= other._mask;
  return *this;
}

TagColorSet TagColorSet::operator&(const TagColorSet & other) const
{
  return TagColorSet(_mask & other._mask);
}

TagColorSet & TagColorSet::operator&=(const TagColorSet & other)
{
  _mask &= other._mask;
  return *this;
}

unsigned int TagColorSet::mask() const
{
  return _mask;
}

TagColorSet::const_iterator::const_iterator(const TagColorSet & set) : _position(0), _set(set)
{
  while (!isAtEnd() && !(_set._mask & (1 << _position))) {
    ++_position;
  }
}

bool TagColorSet::const_iterator::isAtEnd() const
{
  return (_position == int(TagColor::Count));
}

TagColorSet::const_iterator & TagColorSet::const_iterator::operator++()
{
  while (!isAtEnd()) {
    ++_position;
    if (_set._mask & (1 << _position)) {
      break;
    }
  }
  return *this;
}

TagColor TagColorSet::const_iterator::operator*() const
{
  Q_ASSERT_X(!isAtEnd(), __PRETTY_FUNCTION__, "Should not dereference end() iterator");
  Q_ASSERT_X((_set._mask & (1 << _position)), __PRETTY_FUNCTION__, "Current TagColor but is not set");
  return TagColor(_position);
}

TagColorSet::const_iterator TagColorSet::const_iterator::operator++(int)
{
  auto current = *this;
  ++(*this);
  return current;
}

bool TagColorSet::const_iterator::operator!=(const TagColorSet::const_iterator & other) const
{
  return (&_set != &other._set) || (_position != other._position);
}

bool TagColorSet::const_iterator::operator==(const TagColorSet::const_iterator & other) const
{
  return (&_set == &other._set) && (_position == other._position);
}

TagColorSet::const_iterator TagColorSet::begin() const
{
  return const_iterator(*this);
}

TagColorSet::const_iterator TagColorSet::end() const
{
  const_iterator it(*this);
  while (!it.isAtEnd()) {
    ++it;
  }
  return it;
}

inline TagColorSet & TagColorSet::operator=(const TagColorSet & other)
{
  _mask = other._mask;
  return *this;
}

} // namespace GmicQt

#endif // GMIC_QT_TAGS_H
