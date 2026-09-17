#include "scan.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

namespace ui {
namespace {

void ImportImage(QWidget* parent, QLabel* preview, QLabel* status) {
  const QString path = QFileDialog::getOpenFileName(
      parent, QObject::tr("Import image"), QString(),
      QObject::tr("Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;"
                  "All files (*)"));
  if (path.isEmpty()) {
    return;
  }

  QImageReader reader(path);
  reader.setAutoTransform(true);
  const QImage image = reader.read();
  if (image.isNull()) {
    preview->setText(QObject::tr("No image imported"));
    preview->setFixedSize(preview->sizeHint());
    preview->setToolTip(QString());
    status->setText(QObject::tr("Could not load %1: %2")
                        .arg(QFileInfo(path).fileName(), reader.errorString()));
    return;
  }

  QPixmap pixmap = QPixmap::fromImage(image);
  pixmap.setDevicePixelRatio(1.0);
  preview->setPixmap(pixmap);
  preview->setFixedSize(image.size());
  preview->setToolTip(path);
  status->setText(QObject::tr("%1 (%2 × %3)")
                      .arg(QFileInfo(path).fileName())
                      .arg(image.width()).arg(image.height()));
}

}  // namespace

QWidget* CreateScanSection(QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Scan"), parent);
  auto* layout = new QVBoxLayout(section);
  auto* import_button = new QPushButton(QObject::tr("Import image…"), section);
  auto* preview = new QLabel(QObject::tr("No image imported"), section);

  preview->setAlignment(Qt::AlignCenter);
  preview->setAccessibleName(QObject::tr("Imported image preview"));
  preview->adjustSize();

  auto* preview_area = new QScrollArea(section);
  preview_area->setWidgetResizable(false);
  preview_area->setAlignment(Qt::AlignCenter);
  preview_area->setWidget(preview);

  auto* progress = new QProgressBar(section);
  auto* status = new QLabel(QObject::tr("No image imported."), section);
  
  status->setWordWrap(true);
  status->setTextFormat(Qt::PlainText);

  progress->setRange(0, 100);
  progress->setValue(0);

  layout->addWidget(import_button);
  layout->addWidget(preview_area, 1);
  layout->addWidget(progress);
  layout->addWidget(status);

  QObject::connect(import_button, &QPushButton::clicked, section,
                   [section, preview, status]() {
    ImportImage(section, preview, status);
  });
  return section;
}

}  // namespace ui
