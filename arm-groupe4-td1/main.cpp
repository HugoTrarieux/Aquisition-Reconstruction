#include "dicom_viewer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DicomViewer w;
    w.show();

    return a.exec();
}
