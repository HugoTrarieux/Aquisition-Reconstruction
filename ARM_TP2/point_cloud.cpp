#include "point_cloud.h"

PointCloud::PointCloud(DicomData *dicom_data, QWidget *parent) : QOpenGLWidget(parent) {
    this->dicom_data = dicom_data;
}

PointCloud::~PointCloud() {}

void PointCloud::initializeGL(){
    glMatrixMode(GL_PROJECTION);
    glClearColor(0,0,0,1);
}

void PointCloud::paintGL(){
    unsigned char *tab = this->dicom_data->getData();
    glMatrixMode(GL_MODELVIEW);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_POLYGON);
    int z=0;
    int cpt=0;
    for(int i=0; i<this->dicom_data->getNbFiles()-1; i++){
        for(int y=0; y<this->dicom_data->getHeight(); y++){
            for(int x=0; x<this->dicom_data->getWidth(); x++){
                glColor4f(0.0, 1.0, 0.0, 1);
                glVertex3f((float)x/dicom_data->getWidth(), (float)y/dicom_data->getHeight(), (float)z/dicom_data->getNbFiles());
                cpt++;
            }
        }
        z++;
    }
    glEnd();
}