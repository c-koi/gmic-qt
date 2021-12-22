/** -*- mode: c++ ; c-basic-offset: 2 -*-
 * @file   FilterTextTranslator.cpp
 * @author Sebastien Fourey
 * @date   Sep 2020
 *
 * @brief  Definition of the class FilterTextTranslator
 *
 * @copyright
 */
#include "FilterTextTranslator.h"
#include <QDebug>
#include "Common.h"

namespace GmicQt
{
QString FilterTextTranslator::translate(const QString & str)
{
  return QCoreApplication::translate("FilterTextTranslator", str.toUtf8().constData());
}
QString FilterTextTranslator::translate(const QString & str, const QString & filterName)
{
  QByteArray text = str.toUtf8();
  QByteArray comment = filterName.toUtf8();
  QString result = QCoreApplication::translate("FilterTextTranslator", text.constData(), comment.constData());
  if (result == str) {
    return QCoreApplication::translate("FilterTextTranslator", text);
  }
  return result;
}
} // namespace GmicQt
