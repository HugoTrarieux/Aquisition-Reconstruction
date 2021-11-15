#ifndef IMAGE_LABEL_H
#define IMAGE_LABEL_H

#include <QLabel>

class ImageLabel : public QLabel {
  Q_OBJECT
public:
  ImageLabel(QWidget *parent = 0);
  ~ImageLabel();

  void setImg(QImage img);
  void updateContent();

protected slots:
  void resizeEvent(QResizeEvent *event) override;

private:
  QImage raw_img;
  QPixmap pixmap;
};

#endif // IMAGE_LABEL_H
