#include "editor.h"

#include <QtCore/QStandardPaths>
#include <QtGui/QGuiApplication>
#include <QtGui/QImageReader>
#include <QtGui/QImageWriter>
#include <QtGui/QScreen>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSlider>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>

#include "opencv4/opencv2/opencv.hpp"
#include "min_max_pyramid.h"

Editor::Editor(MinMaxPyramid *pyramid, QWidget *parent)
    : QMainWindow(parent), pyramid_(pyramid), image_label_(new QLabel),
      scroll_area_(new QScrollArea) {
  setWindowTitle("UnderSee");

  image_label_->setBackgroundRole(QPalette::Base);
  image_label_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  image_label_->setScaledContents(true);

  scroll_area_->setBackgroundRole(QPalette::Dark);
  // Causes the scroll area to take up the full window, rather than displaying
  // scroll bars. TODO: toggle this later?
  scroll_area_->setWidgetResizable(true);
  // Hide the scroll area until an image is loaded (mostly hides the
  // non-functional scroll bar)
  scroll_area_->setVisible(false);
  setCentralWidget(scroll_area_);

  createActions();

  resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
}

// Stolen boilerplate from QT tutorial
static void initializeImageFileDialog(QFileDialog &dialog,
                                      QFileDialog::AcceptMode acceptMode) {
  static bool firstDialog = true;

  if (firstDialog) {
    firstDialog = false;
    const QStringList picturesLocations =
        QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath()
                                                    : picturesLocations.last());
  }

  dialog.setAcceptMode(acceptMode);
  if (acceptMode == QFileDialog::AcceptSave)
    dialog.setDefaultSuffix("jpg");
}

void Editor::open() {
  QFileDialog dialog(this, tr("Open File"));
  initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

  while (dialog.exec() == QDialog::Accepted &&
         !loadFile(dialog.selectedFiles().constFirst())) {
  }
  // In case we've opened a file while in batch mode, disable key commands for
  // saving: if the user has gone "off-script" (lol) by opening a different
  // file, we don't want to accidentally save over the original destination file
  // by accident. User can still save, but has to use the menu.
  batch_mode_ = false;
}

void Editor::save() {
  QFileDialog dialog(this, tr("Save File As"));
  initializeImageFileDialog(dialog, QFileDialog::AcceptSave);

  while (dialog.exec() == QDialog::Accepted &&
         !saveFile(dialog.selectedFiles().constFirst())) {
  }
}

bool Editor::loadFile(const QString &fileName) {
  cv::Mat image = cv::imread(fileName.toStdString());
  // Note: Qt 5.14 has Format_BGR888 which would let us skip this conversion
  if (image.empty()) {
    QMessageBox::information(
        this, QGuiApplication::applicationDisplayName(),
        tr("Cannot load %1").arg(QDir::toNativeSeparators(fileName)));
    return false;
  }

  setWindowTitle("UnderSee - " + fileName);

  cv_image_ = image;
  cv::cvtColor(cv_image_, cv_image_, cv::COLOR_BGR2RGB);
  pyramid_->PreProcess(cv_image_);
  // See sliderChanged for detail on range
  locality_slider_->setRange(0, pyramid_->MaxScale() - pyramid_->MinScale());
  locality_slider_->setValue(0); // Original image
  QImage newImage(cv_image_.data, cv_image_.cols, cv_image_.rows,
                  static_cast<int>(cv_image_.step), QImage::Format_RGB888);

  image_ = newImage;

  save_action_->setEnabled(true);

  image_label_->setPixmap(QPixmap::fromImage(image_));
  image_label_->adjustSize();
  scroll_area_->setVisible(true);
  return true;
}

bool Editor::saveFile(const QString &fileName) {
  QImageWriter writer(fileName);

  if (!writer.write(image_)) {
    QMessageBox::information(
        this, QGuiApplication::applicationDisplayName(),
        tr("Cannot write %1: %2")
            .arg(QDir::toNativeSeparators(fileName), writer.errorString()));
    return false;
  }
  const QString message =
      tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName));
  statusBar()->showMessage(message);
  return true;
}

void Editor::createActions() {
  QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

  QAction *openAct = fileMenu->addAction(tr("&Open..."), this, &Editor::open);
  openAct->setShortcut(QKeySequence::Open);

  save_action_ = fileMenu->addAction(tr("&Save..."), this, &Editor::save);
  save_action_->setEnabled(false);

  fileMenu->addSeparator();

  QAction *exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
  exitAct->setShortcut(tr("Ctrl+Q"));

  locality_slider_ = new QSlider(Qt::Horizontal);
  locality_slider_->setRange(0, 100);
  locality_slider_->setValue(0);
  locality_slider_->setMinimumSize(300, 50);

  QObject::connect(locality_slider_, &QSlider::valueChanged,
                   [&](int value) { this->localitySliderChanged(value); });

  QLabel *locality_label = new QLabel("Locality");

  QHBoxLayout *slider_hbox = new QHBoxLayout;
  slider_hbox->addWidget(locality_label);
  slider_hbox->addWidget(locality_slider_);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addLayout(slider_hbox);
  layout->addWidget(image_label_);

  scroll_area_->setLayout(layout);
}

void Editor::localitySliderChanged(int value) {
  // It's nice for the UI to go from no change to more severe change. So reverse
  // the direction of the slider, and make sure that the leftmost value is
  // out-of-range (and therefore returns the original image).
  cv_image_ = pyramid_->Relevel(pyramid_->MaxScale() - value);
  QImage newImage(cv_image_.data, cv_image_.cols, cv_image_.rows,
                  static_cast<int>(cv_image_.step), QImage::Format_RGB888);
  image_ = newImage;
  image_label_->setPixmap(QPixmap::fromImage(image_));
}

bool Editor::runAsBatch(const QString &inFileName, const QString &outFileName) {
  batch_mode_ = true;
  batch_input_file_ = inFileName;
  batch_output_file_ = outFileName;

  return loadFile(inFileName);
}

void Editor::keyPressEvent(QKeyEvent *event) {
  if (batch_mode_ && event->key() == Qt::Key_Space) {
    saveFile(batch_output_file_);
    close();
  } else if (batch_mode_ && event->key() == Qt::Key_Escape) {
    // Close without saving
    close();
  } else {
    QMainWindow::keyPressEvent(event);
  }
}
