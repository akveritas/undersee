#include <QtGui/QKeyEvent>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSlider>
#include <memory>

#include "opencv4/opencv2/opencv.hpp"

class MinMaxPyramid;

class Editor : public QMainWindow {
  Q_OBJECT

public:
  explicit Editor(MinMaxPyramid *pyramid, QWidget *parent = nullptr);
  ~Editor() = default;

  // We want to be able to use the same UI to batch run against multiple images.
  // To support this, let the caller initialize with fixed filenames.
  bool runAsBatch(const QString &inFileName, const QString &outFileName);

private slots:
  void open();
  void save();

protected:
  void keyPressEvent(QKeyEvent *event) override;

private:
  void createActions();
  bool loadFile(const QString &fileName);
  bool saveFile(const QString &fileName);

  void localitySliderChanged(int value);

  bool batch_mode_ = false;
  QString batch_input_file_;
  QString batch_output_file_;

  MinMaxPyramid *pyramid_;

  QLabel *image_label_;
  QScrollArea *scroll_area_;
  QSlider *locality_slider_;

  // QImage and cv::Mat can share their underlying data, and are ref-counted.
  // This is convenient, except that they don't count each other as references.
  // To avoid garbage collection problems, we keep one copy of each type;
  // hopefully these usually point to the same buffer, although this class
  // mostly uses QImage, and even then tries not to know anything about its
  // internals.
  cv::Mat cv_image_;
  QImage image_;

  QAction *save_action_;
};
