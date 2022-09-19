#include "JpegQualityDialog.h"
#include <QSettings>
#include "Settings.h"
#include "ui_jpegqualitydialog.h"
int JpegQualityDialog::_permanentQuality = -1;
int JpegQualityDialog::_selectedQuality = -1;

#define JPEG_QUALITY_KEY "Config/host_standalone/DefaultJpegQuality"

JpegQualityDialog::JpegQualityDialog(QWidget * parent) : QDialog(parent), ui(new Ui::JpegQualityDialog)
{
  ui->setupUi(this);
  setWindowTitle(tr("JPEG Quality"));
  ui->slider->setRange(0, 100);
  ui->spinBox->setRange(0, 100);

  if (_selectedQuality == -1) {
    _selectedQuality = QSettings().value(JPEG_QUALITY_KEY, 85).toInt();
  }

  ui->slider->setValue(_selectedQuality);
  ui->spinBox->setValue(_selectedQuality);
  ui->pbOk->setDefault(true);
  connect(ui->slider, &QSlider::valueChanged, ui->spinBox, &QSpinBox::setValue);
  connect(ui->spinBox, QOverload<int>::of(&QSpinBox::valueChanged), ui->slider, &QSlider::setValue);
  connect(ui->pbOk, &QPushButton::clicked, [this]() {
    _selectedQuality = ui->spinBox->value();
    QSettings().setValue(JPEG_QUALITY_KEY, _selectedQuality);
  });
  connect(ui->pbOk, &QPushButton::clicked, this, &QDialog::accept);
  connect(ui->pbCancel, &QPushButton::clicked, this, &QDialog::reject);
  connect(ui->makePermanent, &QCheckBox::toggled, this, &JpegQualityDialog::makePermanent);

  if (GmicQt::Settings::darkThemeEnabled()) {
    QPalette p = ui->makePermanent->palette();
    p.setColor(QPalette::Text, GmicQt::Settings::CheckBoxTextColor);
    p.setColor(QPalette::Base, GmicQt::Settings::CheckBoxBaseColor);
    ui->makePermanent->setPalette(p);
  }
}

JpegQualityDialog::~JpegQualityDialog()
{
  delete ui;
}

int JpegQualityDialog::quality() const
{
  return ui->slider->value();
}

void JpegQualityDialog::setQuality(int q)
{
  if (q != -1) {
    ui->slider->setValue(q);
    ui->spinBox->setValue(q);
  }
}

int JpegQualityDialog::ask(QWidget * parent, int value)
{
  if (_permanentQuality != -1) {
    return _permanentQuality;
  }
  JpegQualityDialog * dialog = new JpegQualityDialog(parent);
  dialog->setQuality(value);
  int result = -1;
  if (dialog->exec() == QDialog::Accepted) {
    result = dialog->quality();
  }
  dialog->deleteLater();
  return result;
}

void JpegQualityDialog::closeEvent(QCloseEvent * event)
{
  return QDialog::closeEvent(event);
}

void JpegQualityDialog::makePermanent(bool on)
{
  if (on) {
    _permanentQuality = ui->spinBox->value();
  } else {
    _permanentQuality = -1;
  }
}
