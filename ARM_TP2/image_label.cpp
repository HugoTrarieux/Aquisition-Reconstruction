#include "image_label.h"

ImageLabel::ImageLabel(QWidget *parent) : QLabel(parent) {}

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
  // pixmap = QPixmap::fromImage(raw_img);
  setPixmap(pixmap);
}

void ImageLabel::resizeEvent(QResizeEvent *event) {
  (void)event;
  updateContent();
}
