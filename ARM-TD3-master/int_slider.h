#ifndef INT_SLIDER_H
#define INT_SLIDER_H

#include <QLabel>
#include <QSlider>
#include <QWidget>

/// A class implementing a slider for int values with:
/// - A name shown in a label
/// - An inner slider based on ints
/// - The value shown in a label
///
/// It features customizable limits
class IntSlider : public QWidget {
  Q_OBJECT
public:
  IntSlider(const QString &slider_name, int min_val, int max_val,
            QWidget *parent = nullptr);
  ~IntSlider();

  int value();

public slots:
  /// Set the value of the slider, updates the label and emit a valueChanged
  /// signal
  void setValue(int new_value);
  /// Slots called when the inner value of the slider changes
  void onValueChange(int new_value);
  void setRange(int min, int max);

signals:
  void valueChanged(int new_value);

private:
  QLayout *layout;
  QLabel *name_label;
  QSlider *slider;
  QLabel *value_label;

  void updateValueLabel();
};

#endif // INT_SLIDER_H
