#include <stdlib.h>

#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QSlider>
#include <QtWidgets/QVBoxLayout>
#include <chrono>
#include <fstream>
#include <iostream>

#include "editor.h"
#include "min_max_pyramid.h"
#include "opencv4/opencv2/opencv.hpp"

int relevel_scale = 0;
cv::Mat display_image;
std::string window_title;

int main(int argc, char **argv) {
  MinMaxPyramid pyramid;
  QApplication app(argc, argv);
  Editor editor(&pyramid);

  // If we have arguments, process in batch mode; otherwise, interactive.
  if (argc == 3) {
    if (!editor.runAsBatch(argv[1], argv[2])) {
      std::cerr << "Failed to inialize; bad batch arguments?\n\n";
      return 1;
    }
  }

  if (argc == 2) {
    std::cerr << "Batch usage: undersee <input_filename> <output_filename>\n\n";
  }
  editor.show();
  return app.exec();
}
