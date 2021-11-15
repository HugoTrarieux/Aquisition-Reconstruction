#include "image_label.h"

ImageLabel::ImageLabel(QWidget *parent) : QLabel(parent) {
  QSizePolicy size_policy;
  size_policy.setVerticalPolicy(QSizePolicy::MinimumExpanding);
  size_policy.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
  setSizePolicy(size_policy);
  setMinimumSize(200, 200);
}

ImageLabel::~ImageLabel() {}

void ImageLabel::setImg(QImage img) {
  raw_img = img;
  updateContent();
}

void ImageLabel::updateContent() {
  if (raw_img.isNull())
    return;
  QPixmap tmp_pixmap = QPixmap::fromImage(raw_img);
  pixmap = tmp_pixmap.scaled(this->size(), Qt::KeepAspectRatio);
  setPixmap(pixmap);
}

void ImageLabel::resizeEvent(QResizeEvent *event) {
  (void)event;
  updateContent();
}
