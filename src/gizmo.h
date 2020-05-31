#ifndef GIZMO_H
#define GIZMO_H

#include <QGLViewer/vec.h>
#include <eigen3/Eigen/SparseCore>

class Gizmo{
public:
    qglviewer::Vec origin;
    Eigen::MatrixXf transforMatrix;


    void setOrigin(qglviewer::Vec origin){
        this->origin = origin;
    }

    qglviewer::Vec getOrigin(){
        return this->origin;
    }

    Eigen::MatrixXf getMatrix(){
        return transforMatrix;
    }

    void setTransfoMatrix(qglviewer::Vec newPosition, qreal rotation[3][3]){
        for(int i=0; i < 3; i++){
            for(int j = 0; j < 3; j++){
                this->transforMatrix(i,j) = rotation[i][j];
            }
        }
        transforMatrix(0,3)= newPosition[0] - this->origin[0];
        transforMatrix(1,3)= newPosition[1] - this->origin[1];
        transforMatrix(2,3)= newPosition[2] - this->origin[2];

        transforMatrix(3,0) = 0;
        transforMatrix(3,1) = 0;
        transforMatrix(3,2) = 0;
        transforMatrix(3,3) = 1;

    }


    Gizmo(qglviewer::Vec newPosition, qreal rotation[3][3], qglviewer::Vec origin){
        this->transforMatrix.resize(4,4);
        this->origin = origin;
        this->setTransfoMatrix(newPosition, rotation);
    }

    Gizmo(){
        this->transforMatrix.resize(4,4);
        this->origin = qglviewer::Vec(0,0,0);
    }
};





#endif // GIZMO_H
