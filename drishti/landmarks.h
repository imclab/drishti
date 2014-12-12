#ifndef LANDMARKS_H
#define LANDMARKS_H

#include <QGLViewer/manipulatedCameraFrame.h>
#include <QGLViewer/mouseGrabber.h>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "commonqtclasses.h"

#include <QTableWidget>
#include <QHeaderView>

class Landmarks : public QObject, public MouseGrabber
{
 Q_OBJECT

 public :
  Landmarks();
  ~Landmarks();

    enum MoveAxis
  {
    MoveX,
    MoveY,
    MoveZ,
    MoveAll
  };

  void setPoints(QList<Vec>);
  void addPoints(QList<Vec>);
  void setPoint(int, Vec);

  Vec point(int);
  QList<Vec> points();

  void setPointSize(int);
  int pointSize();

  void setPointColor(Vec);
  Vec pointColor();

  int count();
  void clear();

  int moveAxis();
  void setMoveAxis(int);

  bool grabsMouse();
  
  void checkIfGrabsMouse(int, int, const Camera* const);
  void mousePressEvent(QMouseEvent* const, Camera* const);
  void mouseMoveEvent(QMouseEvent* const, Camera* const);
  void mouseReleaseEvent(QMouseEvent* const, Camera* const);

  bool keyPressEvent(QKeyEvent*);

  void draw(QGLViewer*, bool);
  void postdraw(QGLViewer*);

  void setMouseGrab(bool);
  void toggleMouseGrab();

 public slots :
  void updateLandmarks(); 
  void updateLandmarks(int, int); 
  void saveLandmarks();
  void loadLandmarks();
  void loadLandmarks(QString);
  void clearAllLandmarks();

 signals :
  void updateGL();

 private :
  int m_moveAxis;
  int m_grabbed;
  int m_pressed;
  QPoint m_prevPos;

  bool m_grab;

  Vec m_pointColor;
  int m_pointSize;

  QList<Vec> m_points;
  QList<QString> m_text;

  bool m_showCoordinates;
  bool m_showText;
  bool m_showNumber;

  QWidget *m_table;
  QTableWidget *m_tableWidget;


  void updateTable();
};




#endif