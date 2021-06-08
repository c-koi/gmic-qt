#ifndef JPEGQUALITYDIALOG_H
#define JPEGQUALITYDIALOG_H

#include <QDialog>

namespace Ui
{
class JpegQualityDialog;
}

class JpegQualityDialog : public QDialog {
  Q_OBJECT

public:
  explicit JpegQualityDialog(QWidget * parent = nullptr);
  ~JpegQualityDialog() override;
  int quality() const;
  void setQuality(int quality);
  static int ask(QWidget * parent, int value = -1);

protected:
  void closeEvent(QCloseEvent *) override;
private slots:
  void makePermanent(bool);

private:
  Ui::JpegQualityDialog * ui;
  static int _permanentQuality;
  static int _selectedQuality;
};

#endif // JPEGQUALITYDIALOG_H
