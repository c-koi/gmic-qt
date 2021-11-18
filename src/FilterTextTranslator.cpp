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
namespace GmicQt
{
QString FilterTextTranslator::translate(const QString & str)
{
  QByteArray array = str.toUtf8();
  return QCoreApplication::translate("FilterTextTranslator", array.constData());
}
} // namespace GmicQt
