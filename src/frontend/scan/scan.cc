#include "scan.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QPixmap>
#include <QHBoxLayout>
#include <QStyle>

#include "laser_control/laser_control.h"
#include <QPushButton>
#include <QPainter>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

namespace ui {
namespace {

class ImagePreview : public QLabel {
 public:
  explicit ImagePreview(QWidget* parent) : QLabel(parent) {
    setText(tr("No image imported"));
    setAlignment(Qt::AlignCenter);
    setMinimumSize(180, 180);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
  }

 protected:
  void paintEvent(QPaintEvent* event) override {
    const QPixmap image = pixmap();
    if (image.isNull()) {
      QLabel::paintEvent(event);
      return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const QSize fitted = image.size().scaled(contentsRect().size(), Qt::KeepAspectRatio);
    const QRect target(contentsRect().center() - QPoint(fitted.width() / 2, fitted.height() / 2), fitted);
    painter.drawPixmap(target, image);
  }
};

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
    preview->setToolTip(QString());
    status->setText(QObject::tr("Could not load %1: %2")
                        .arg(QFileInfo(path).fileName(), reader.errorString()));
    return;
  }

  QPixmap pixmap = QPixmap::fromImage(image);
  pixmap.setDevicePixelRatio(1.0);
  preview->setPixmap(pixmap);

  preview->setToolTip(path);
  status->setText(QObject::tr("%1 (%2 × %3)")
                      .arg(QFileInfo(path).fileName())
                      .arg(image.width()).arg(image.height()));
}

}  // namespace

QWidget* CreateImageSection(QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr(" Image"), parent);

  auto* layout = new QVBoxLayout(section);
  auto* import_button = new QPushButton(QObject::tr(" Import"), section);
  import_button->setIcon(section->style()->standardIcon(QStyle::SP_FileIcon));

  auto* clear_button = new QPushButton(QObject::tr("Clear"), section);

  auto* toolbar = new QHBoxLayout();
  for (auto* button : {import_button, clear_button}) {
    button->setMinimumWidth(88);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  }
  toolbar->addWidget(import_button, 1);
  toolbar->addWidget(clear_button, 1);

  auto* preview = new ImagePreview(section);

  preview->setAlignment(Qt::AlignCenter);
  preview->setAccessibleName(QObject::tr("Imported image preview"));

  auto* status = new QLabel(QObject::tr("No image imported."), section);

  status->setWordWrap(true);
  status->setTextFormat(Qt::PlainText);

  layout->addLayout(toolbar);
  layout->addWidget(preview, 1);

  layout->addWidget(status);

  QObject::connect(import_button, &QPushButton::clicked, section,
                   [section, preview, status]() {
    ImportImage(section, preview, status);
  });

  QObject::connect(clear_button, &QPushButton::clicked, section, [preview, status]() {
    preview->clear();
    preview->setText(QObject::tr("No image imported"));
    preview->setToolTip(QString());
    status->setText(QObject::tr("No image imported."));
  });

  return section;
}

QWidget* CreateScanSection(QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Scan"), parent);

  auto* layout = new QHBoxLayout(section);
  layout->setSpacing(12);
  layout->addWidget(CreateImageSection(section), 3);
  layout->addWidget(CreateLaserControlSection(section), 2);

  return section;
}

}  // namespace ui
