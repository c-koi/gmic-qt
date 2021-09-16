/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterTreeItemDelegate.cpp
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
#include "FilterTreeItemDelegate.h"
#include <QColor>
#include <QDebug>
#include <QPainter>
#include <QPalette>
#include <QTextDocument>
#include "DialogSettings.h"
#include "FilterSelector/FiltersView/FilterTreeAbstractItem.h"
#include "FilterSelector/FiltersView/FilterTreeItem.h"
#include "Tags.h"

namespace GmicQt
{

FilterTreeItemDelegate::FilterTreeItemDelegate(QObject * parent) : QStyledItemDelegate(parent) {}

void FilterTreeItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
  QStyleOptionViewItem options = option;
  initStyleOption(&options, index);
  painter->save();

  auto model = dynamic_cast<const QStandardItemModel *>(index.model());
  Q_ASSERT_X(model, "FiltersTreeItemDelegate::paint()", "No model");
  const QStandardItem * item = model->itemFromIndex(index);
  Q_ASSERT_X(item, "FiltersTreeItemDelegate::paint()", "No item");
  auto filter = dynamic_cast<const FilterTreeItem *>(item);

  const int width = 0.5 * options.rect.height();
  QString tag = TagAssets::markerHtml(TagColor::Green, width);

  QTextDocument doc;
  if (!item->isCheckable() && filter && !filter->isVisible()) {
    QColor textColor;
    textColor = DialogSettings::UnselectedFilterTextColor;
    doc.setHtml(QString("<span style=\"color:%1\">%2</span>&nbsp;%3").arg(textColor.name()).arg(options.text).arg(tag));
  } else {
    if (filter) {
      doc.setHtml(options.text + "&nbsp;" + tag);
    } else {
      doc.setHtml(options.text);
    }
  }
  options.text = "";
  options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);
  painter->translate(options.rect.left(), options.rect.top());
  QRect clip(0, 0, options.rect.width(), options.rect.height());
  doc.drawContents(painter, clip);
  painter->restore();
}

QSize FilterTreeItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
  QStyleOptionViewItem options = option;
  initStyleOption(&options, index);

  QTextDocument doc;
  doc.setHtml(options.text);
  doc.setTextWidth(options.rect.width());
  return {static_cast<int>(doc.idealWidth()), static_cast<int>(doc.size().height())};
}

} // namespace GmicQt
