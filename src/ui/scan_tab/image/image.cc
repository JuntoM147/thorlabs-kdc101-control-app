#include "scan_tab/image/image.h"
#include "scan_tab/route_preview/route_preview.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QPixmap>
#include <QHBoxLayout>
#include <QStyle>

#include <QPushButton>
#include <QPainter>
#include <QTransform>
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

  // Layout follows the available window space, never the source image dimensions.
  QSize sizeHint() const override { return QSize(320, 240); }
  QSize minimumSizeHint() const override { return QSize(180, 180); }

  void SetRoute(const algo::Program& program, algo::PixelPosition start) {
    route_preview_.SetRoute(program, start);
    update();
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
    constexpr int kImagePadding = 12;
    const QRect available = contentsRect().adjusted(kImagePadding, kImagePadding,
                                                   -kImagePadding, -kImagePadding);
    if (available.isEmpty()) return;
    const QSize fitted = image.size().scaled(available.size(), Qt::KeepAspectRatio);
    const QRect target(available.topLeft() + QPoint((available.width() - fitted.width()) / 2,
                                                  (available.height() - fitted.height()) / 2), fitted);
    painter.drawPixmap(target, image);
    // Allow edge markers to extend into the padding without being cut off.
    painter.setClipRect(contentsRect());
    QTransform transform;
    transform.translate(target.x(), target.y());
    transform.scale(double(target.width()) / image.width(), double(target.height()) / image.height());
    route_preview_.Draw(painter, transform);
  }

 private:
  RoutePreview route_preview_;
};

void ImportImage(MainWindow& view, QWidget* parent, QLabel* preview, QLabel* status) {
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
    view.SetScanImage({});
    preview->clear();
    preview->setText(QObject::tr("No image imported"));
    preview->setToolTip(QString());
    const auto message = QObject::tr("Could not load %1: %2")
        .arg(QFileInfo(path).fileName(), reader.errorString());
    status->setText(message);
    view.ShowMessage(message, true);
    return;
  }

  view.SetScanImage(image);

  preview->setToolTip(path);
  status->setText(QObject::tr("%1 (%2 × %3)")
                      .arg(QFileInfo(path).fileName())
                      .arg(image.width()).arg(image.height()));
}

}  // namespace

QWidget* CreateImageSection(MainWindow& view, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Scan preview"), parent);

  section->setObjectName(QStringLiteral("image"));
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

  QObject::connect(&view, &MainWindow::ScanImageChanged, preview, [preview, status](const QImage& image) {
    preview->setToolTip(QString());
    if (image.isNull()) {
      preview->clear();
      preview->setText(QObject::tr("No image imported"));
      status->setText(QObject::tr("No image imported."));
    } else {
      QPixmap pixmap = QPixmap::fromImage(image);
      pixmap.setDevicePixelRatio(1.0);
      preview->setPixmap(pixmap);
      status->setText(QObject::tr("Image (%1 × %2)").arg(image.width()).arg(image.height()));
    }
  });
  QObject::connect(&view, &MainWindow::RoutePreviewChanged, preview, [&view, preview] {
    preview->SetRoute(view.PreviewProgram(), view.StartPixel());
  });

  layout->addWidget(status);

  QObject::connect(import_button, &QPushButton::clicked, section,
                   [&view, section, preview, status]() {
    ImportImage(view, section, preview, status);
  });

  QObject::connect(clear_button, &QPushButton::clicked, section, [&view, preview, status]() {
    view.SetScanImage({});
    preview->clear();
    preview->setText(QObject::tr("No image imported"));
    preview->setToolTip(QString());
    status->setText(QObject::tr("No image imported."));
    view.ShowMessage(QObject::tr("Image cleared."));
  });

  QObject::connect(&view, &MainWindow::ScanInputsEnabled, import_button, &QWidget::setEnabled);
  QObject::connect(&view, &MainWindow::ScanInputsEnabled, clear_button, &QWidget::setEnabled);
  return section;
}

}  // namespace ui
