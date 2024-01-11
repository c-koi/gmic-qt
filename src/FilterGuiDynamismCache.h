/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterGuiDynamismCache.h
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
#ifndef GMIC_QT_FILTERGUIDYNAMISMCACHE_H
#define GMIC_QT_FILTERGUIDYNAMISMCACHE_H

#include <QHash>
#include <QString>

namespace GmicQt
{

enum FilterGuiDynamism
{
  Unknown = 0,
  Static = 1,
  Dynamic = 2
};

class FilterGuiDynamismCache {
public:
  static void load();
  static void save();
  static void setValue(const QString & hash, FilterGuiDynamism dynamism);
  static FilterGuiDynamism getValue(const QString & hash);
  static void remove(const QString & hash);
  static void clear();

private:
  static QHash<QString, int> _dynamismCache;
};

} // namespace GmicQt

#endif // GMIC_QT_FILTERGUIDYNAMISMCACHE_H
