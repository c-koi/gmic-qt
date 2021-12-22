/** -*- mode: c++ ; c-basic-offset: 2 -*-
 * @file   FilterTextTranslator.h
 * @author Sebastien Fourey
 * @date   Sep 2020
 *
 * @brief  Declaration of the class FilterTextTranslator
 *
 * @copyright
 */
#ifndef GMIC_QT_FILTERTEXTTRANSLATOR_H
#define GMIC_QT_FILTERTEXTTRANSLATOR_H

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QObject>
#include <QString>
#include "Common.h"
namespace GmicQt
{
/**
 *  The FilterTextTranslator class.
 */
class FilterTextTranslator : public QObject {
  Q_OBJECT
public:
  FilterTextTranslator() = delete;
  static QString translate(const QString & str);
  static QString translate(const QString & str, const QString & filterName);

protected:
private:
};

} // namespace GmicQt

#endif // GMIC_QT_FILTERTEXTTRANSLATOR_H
