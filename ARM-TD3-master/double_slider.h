#ifndef DOUBLE_SLIDER_H
#define DOUBLE_SLIDER_H

#include <QLabel>
#include <QSlider>
#include <QWidget>

/// A class implementing a slider for double values with:
/// - A name shown in a label
/// - An inner slider based on ints
/// - The value shown in a label
///
/// It features customizable limits
class DoubleSlider : public QWidget {
  Q_OBJECT
public:
  DoubleSlider(const QString &slider_name, double min_val, double max_val,
               QWidget *parent = 0, int nb_steps = 1000);
  ~DoubleSlider();

  void setLimits(double min, double max);

  double value();

  void setValue(double new_value);

public slots:
  void onSliderChanged(int new_value);

signals:
  void valueChanged(double new_value);

private:
  QLayout *layout;
  QLabel *name_label;
  QSlider *slider;
  /// TODO replace by a QTextBox
  QLabel *value_label;

  double min_val;
  double max_val;

  void updateValueLabel();
  double innerToValue(int slider_value);
  int valueToInner(double value);
};

#endif // DOUBLE_SLIDER_H
