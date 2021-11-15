#include "double_slider.h"

#include <QHBoxLayout>
#include <cmath>
#include <sstream>

DoubleSlider::DoubleSlider(const QString &slider_name, double min_val,
                           double max_val, QWidget *parent, int nb_steps)
    : QWidget(parent), min_val(min_val), max_val(max_val) {
  QSizePolicy size_policy;
  size_policy.setVerticalPolicy(QSizePolicy::Fixed);
  size_policy.setHorizontalPolicy(QSizePolicy::Minimum);
  setSizePolicy(size_policy);
  layout = new QHBoxLayout(this);
  name_label = new QLabel(slider_name);
  slider = new QSlider(Qt::Horizontal);
  slider->setMinimum(0);
  slider->setMaximum(nb_steps);
  value_label = new QLabel("value");
  layout->addWidget(name_label);
  layout->addWidget(slider);
  layout->addWidget(value_label);

  connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged(int)));
  updateValueLabel();
}

DoubleSlider::~DoubleSlider() {}

double DoubleSlider::value() { return innerToValue(slider->value()); }

void DoubleSlider::setValue(double new_value) {
  slider->setValue(valueToInner(new_value));
}

void DoubleSlider::setLimits(double min, double max) {
  double new_value = value();
  min_val = min;
  max_val = max;
  if (min_val > new_value)
    new_value = min_val;
  if (max_val < new_value)
    new_value = max_val;
  slider->setValue(valueToInner(new_value));
  updateValueLabel();
}

void DoubleSlider::onSliderChanged(int new_value) {
  double value = innerToValue(new_value);
  updateValueLabel();
  emit valueChanged(value);
}

void DoubleSlider::updateValueLabel() {
  std::ostringstream oss;
  oss << value() << " [" << min_val << "," << max_val << "]";
  value_label->setText(QString(oss.str().c_str()));
}

double DoubleSlider::innerToValue(int slider_value) {
  return min_val + (max_val - min_val) * slider_value / slider->maximum();
}

int DoubleSlider::valueToInner(double value) {
  double delta = value - min_val;
  double range = max_val - min_val;
  return std::round(delta / range * slider->maximum());
}
