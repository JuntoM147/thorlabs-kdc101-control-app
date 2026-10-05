#include "scan_tab/image/image.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>

#include "scan_tab/scan_preview/scan_preview.h"

namespace ui {
namespace {

void ImportImage(MainWindow& view, QWidget* parent, QLabel* preview,
                 QLabel* status) {
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
    const auto message =
        QObject::tr("Could not load %1: %2")
            .arg(QFileInfo(path).fileName(), reader.errorString());
    status->setText(message);
    view.ShowMessage(message, true);
    return;
  }

  view.SetScanImage(image);

  preview->setToolTip(path);
  status->setText(QObject::tr("%1 (%2 × %3)")
                      .arg(QFileInfo(path).fileName())
                      .arg(image.width())
                      .arg(image.height()));
}

}  // namespace

QWidget* CreateImageSection(MainWindow& view, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Scan preview"), parent);

  section->setObjectName(QStringLiteral("image"));
  auto* layout = new QVBoxLayout(section);
  auto* import_button = new QPushButton(QObject::tr(" Import"), section);
  import_button->setIcon(section->style()->standardIcon(QStyle::SP_FileIcon));

  auto* clear_button = new QPushButton(QObject::tr("Clear image"), section);

  auto* toolbar = new QHBoxLayout();
  for (auto* button : {import_button, clear_button}) {
    button->setMinimumWidth(88);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  }
  toolbar->addWidget(import_button, 1);
  toolbar->addWidget(clear_button, 1);
  auto* preview = new ScanPreview(section);

  preview->setAlignment(Qt::AlignCenter);
  preview->setAccessibleName(QObject::tr("Imported image preview"));

  auto* status = new QLabel(QObject::tr("No image imported."), section);

  status->setWordWrap(true);
  status->setTextFormat(Qt::PlainText);

  layout->addLayout(toolbar);
  layout->addWidget(preview, 1);
  QObject::connect(&view, &MainWindow::StartPixelChanged, preview,
                   &ScanPreview::SetStart);
  QObject::connect(&view, &MainWindow::ScanPreviewStarted, preview,
                   &ScanPreview::FollowInstructions);
  QObject::connect(&view, &MainWindow::ScanDisplayChanged, preview,
                   [preview](ScanState state) {
                     const auto count = state.completed_instructions +
                                        (state.phase == ScanPhase::kRunning &&
                                                 state.completed_instructions <
                                                     state.total_instructions
                                             ? 1
                                             : 0);
                     preview->ShowThrough(count);
                   });
  QObject::connect(&view, &MainWindow::ScanPreviewFinished, preview,
                   &ScanPreview::EndFollowing);
  QObject::connect(&view, &MainWindow::PreviewScanRequested, preview,
                   [&view, preview] {
                     if (view.CanPreviewScan())
                       preview->Start(view.PreviewInstructions());
                   });
  QObject::connect(&view, &MainWindow::StopPreviewRequested, preview,
                   &ScanPreview::Stop);
  QObject::connect(&view, &MainWindow::ClearPreviewRequested, preview,
                   &ScanPreview::Clear);
  QObject::connect(&view, &MainWindow::ScanAvailabilityChanged, preview,
                   [&view, preview] {
                     if (!view.CanPreviewScan()) preview->Stop();
                   });
  QObject::connect(&view, &MainWindow::ScanImageChanged, preview,
                   [preview, status](const QImage& image) {
                     preview->setToolTip(QString());
                     if (image.isNull()) {
                       preview->clear();
                       preview->setText(QObject::tr("No image imported"));
                       status->setText(QObject::tr("No image imported."));
                     } else {
                       QPixmap pixmap = QPixmap::fromImage(image);
                       pixmap.setDevicePixelRatio(1.0);
                       preview->setPixmap(pixmap);
                       status->setText(QObject::tr("Image (%1 × %2)")
                                           .arg(image.width())
                                           .arg(image.height()));
                     }
                   });

  layout->addWidget(status);

  QObject::connect(import_button, &QPushButton::clicked, section,
                   [&view, section, preview, status]() {
                     ImportImage(view, section, preview, status);
                   });

  QObject::connect(clear_button, &QPushButton::clicked, section,
                   [&view, preview, status]() {
                     view.SetScanImage({});
                     preview->clear();
                     preview->setText(QObject::tr("No image imported"));
                     preview->setToolTip(QString());
                     status->setText(QObject::tr("No image imported."));
                     view.ShowMessage(QObject::tr("Image cleared."));
                   });

  QObject::connect(&view, &MainWindow::ScanInputsEnabled, import_button,
                   &QWidget::setEnabled);
  QObject::connect(&view, &MainWindow::ScanInputsEnabled, clear_button,
                   &QWidget::setEnabled);
  return section;
}

}  // namespace ui
