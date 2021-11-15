#include "checkbox.h"

#include <QHBoxLayout>

CheckBox::CheckBox(const QString &box_name, const QString &box_text, QWidget *parent) 
: QWidget(parent) {
    layout = new QHBoxLayout(this);
    name_label = new QLabel(box_name);
    checkBox = new QCheckBox(box_text, parent);
    layout->addWidget(checkBox);
    connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(onValueChange(int)));
}

CheckBox::~CheckBox() {}

bool CheckBox::value() {
    return checkBox->isTristate(); 
}

void CheckBox::setValue(int new_value) {
    
    checkBox->setTristate(new_value); 
}

void CheckBox::onValueChange(int new_value) {
    emit stateChanged(new_value);
}