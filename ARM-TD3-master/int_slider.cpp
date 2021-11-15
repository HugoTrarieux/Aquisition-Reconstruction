#include "int_slider.h"

#include <QHBoxLayout>
#include <cmath>
#include <sstream>

IntSlider::IntSlider(const QString &slider_name, int min_val, int max_val,
                     QWidget *parent)
    : QWidget(parent) {
  QSizePolicy size_policy;
  size_policy.setVerticalPolicy(QSizePolicy::Fixed);
  size_policy.setHorizontalPolicy(QSizePolicy::Minimum);
  setSizePolicy(size_policy);
  layout = new QHBoxLayout(this);
  name_label = new QLabel(slider_name);
  slider = new QSlider(Qt::Horizontal);
  slider->setMinimum(min_val);
  slider->setMaximum(max_val);
  value_label = new QLabel("value");
  layout->addWidget(name_label);
  layout->addWidget(slider);
  layout->addWidget(value_label);

  connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onValueChange(int)));
  updateValueLabel();
}

IntSlider::~IntSlider() {}

int IntSlider::value() { return slider->value(); }

void IntSlider::setValue(int new_value) { slider->setValue(new_value); }

void IntSlider::setRange(int min, int max) { slider->setRange(min, max); }

void IntSlider::onValueChange(int new_value) {
  updateValueLabel();
  emit valueChanged(new_value);
}

void IntSlider::updateValueLabel() {
  std::ostringstream oss;
  oss << value() << " [" << slider->minimum() << "," << slider->maximum()
      << "]";
  value_label->setText(QString(oss.str().c_str()));
}
