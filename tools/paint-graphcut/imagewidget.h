#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QWidget>
#include "commonqtclasses.h"
#include <QStatusBar>
#include <QScrollBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVector3D>

#include "livewire.h"
#include "curvegroup.h"

class ImageWidget : public QWidget
{
  Q_OBJECT

 public :
  ImageWidget(QWidget*, QStatusBar *);

  enum SliceType {
    DSlice = 0,
    WSlice,
    HSlice
  };

  void saveImage();
  void loadCurves(QString);
  
  void setGridSize(int, int, int);
  void setSliceType(int);
  void setImage(uchar*, uchar*);
  void setMaskImage(uchar*);

  void setWeightLoG(float);
  void setWeightG(float);
  void setWeightN(float);

  void setRawValue(QList<uchar>);

  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);

  void mouseDoubleClickEvent(QMouseEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent*);
  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);

  void loadLookupTable(QImage);

  void depthUserRange(int&, int&);
  void widthUserRange(int&, int&);
  void heightUserRange(int&, int&);

  void setScrollBars(QScrollBar*, QScrollBar*);
  void getBox(int&, int&, int&, int&, int&, int&);

  void processPrevSliceTags();

  QVector3D pickedPoint() { return QVector3D(m_lastPickDepth, m_lastPickWidth, m_lastPickHeight); }

  QMultiMap<int, Curve*>* multiMapCurvesD() { return m_dCurves.multiMapCurves(); }
  QList< QMap<int, Curve> >* listMapCurvesD() { return m_dCurves.listMapCurves(); }
  QMultiMap<int, Curve*>* multiMapCurvesW() { return m_wCurves.multiMapCurves(); }
  QList< QMap<int, Curve> >* listMapCurvesW() { return m_wCurves.listMapCurves(); }
  QMultiMap<int, Curve*>* multiMapCurvesH() { return m_hCurves.multiMapCurves(); }
  QList< QMap<int, Curve> >* listMapCurvesH() { return m_hCurves.listMapCurves(); }

 public slots :
  void updateTagColors();
  void sliceChanged(int);
  void userRangeChanged(int, int);
  void showHelp();
  void smooth(int, bool);
  void keyPressEvent(QKeyEvent*);
  void setLivewire(bool);
  void setCurve(bool);
  void saveCurves();
  void loadCurves();
  void setSmoothType(int);
  void setGradType(int);
  void freezeLivewire(bool);
  void newCurve();
  void morphCurves();
  void deleteAllCurves();
  void zoom0();
  void zoom9();
  void zoomUp();
  void zoomDown();
  void setPointSize(int);

  void paintUsingCurves(int, int, int, int, uchar*, QList<int>);

  void modifyUsingLivewire();
  void freezeModifyUsingLivewire();

 signals :
  void getSlice(int);
  void getRawValue(int, int, int);
  void newMinMax(float, float);
  void tagDSlice(int, QImage);
  void tagWSlice(int, QImage);
  void tagHSlice(int, QImage);
  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);
  void fillVolume(int, int,
		  int, int,
		  int, int,
		  QList<int>,
		  bool);
  void tagAllVisible(int, int,
		     int, int,
		     int, int);

  void applySmooth(int, bool);
  void simulateKeyPressEvent(QKeyEvent*);
  void applyMaskOperation(int, int, int);
  void polygonLevels(QList<int>);
  void updateViewerBox(int, int, int, int, int, int);

 private :
  QStatusBar *m_statusBar;
  QScrollBar *m_hbar, *m_vbar;

  int m_currSlice;

  bool m_livewireMode;
  LiveWire m_livewire;

  bool m_curveMode;
  CurveGroup m_dCurves;
  CurveGroup m_wCurves;
  CurveGroup m_hCurves;

  QVector<QRgb> m_tagColors;
  QVector<QRgb> m_prevslicetagColors;

  int m_sliceType;
  int m_minDSlice, m_maxDSlice;
  int m_minWSlice, m_maxWSlice;
  int m_minHSlice, m_maxHSlice;
  int m_Depth, m_Width, m_Height;

  float m_zoom;

  int m_imgHeight, m_imgWidth;
  int m_simgHeight, m_simgWidth;
  int m_simgX, m_simgY;

  QImage m_image;
  QImage m_imageScaled;

  QImage m_maskimage;
  QImage m_maskimageScaled;

  QImage m_userimage;
  QImage m_userimageScaled;

  QImage m_prevslicetagimage;
  QImage m_prevslicetagimageScaled;

  QImage m_gradImageScaled;

  uchar *m_slice;
  uchar *m_sliceImage;

  uchar *m_maskslice;

  uchar *m_lut;

  int m_button;
  bool m_pickPoint;
  int m_pickDepth, m_pickWidth, m_pickHeight;
  int m_lastPickDepth, m_lastPickWidth, m_lastPickHeight;

  QList<uchar> m_vgt;
  QPoint m_cursorPos;

  QRectF m_rubberBand;
  bool m_rubberBandActive;
  bool m_rubberXmin;
  bool m_rubberYmin;
  bool m_rubberXmax;
  bool m_rubberYmax;
  bool m_rubberNew;

  uchar *m_prevtags;
  uchar *m_usertags;
  uchar *m_prevslicetags;
  uchar *m_tags;

  bool m_applyRecursive;
  bool m_extraPressed;
  int m_cslc, m_maxslc;
  int m_key;
  bool m_forward;

  int m_pointSize;

  void resizeImage();
  void recolorImage();
  
  void drawSizeText(QPainter*, int, int);
  void drawRawValue(QPainter*);
  void processCommands(QString);
  void drawRubberBand(QPainter*);
  int  drawCurves(QPainter*);
  int  drawMorphedCurves(QPainter*);
  void drawOtherCurvePoints(QPainter*);
  void drawLivewire(QPainter*);

  bool checkRubberBand(int, int);

  void updateRubberBand(int, int);
  bool validPickPoint(int, int);
  
  bool withinBounds(int, int);
  void dotImage(int, int, bool);
  void removeDotImage(int, int);

  void applyGraphCut();
  void applyPaint(bool);
  void applyReset();
  
  void setZoom(float);
  void preselect();

  void updateMaskImage();

  void tagUntaggedRegion();

  void getSliceLimits(int&, int&, int&, int&, int&, int&);

  void applyRecursive(int);
  void checkRecursive();
  
  void paintUsingCurves(CurveGroup*, int, int, int, uchar*, QList<int>);
  void paintUsingCurves(uchar*);

  void saveCurves(QFile*, CurveGroup*);
  void loadCurves(QFile*, CurveGroup*);

  void saveMorphedCurves(QFile*, CurveGroup*);
  void loadMorphedCurves(QFile*, CurveGroup*);

  void saveCurveData(QFile*, int, Curve*);
  QPair<int, Curve> loadCurveData(QFile*);

  bool curveModeKeyPressEvent(QKeyEvent*);

  void propagateLivewire();

  QList<QPoint> trimPointList(QList<QPoint>, bool);

  void startLivewirePropagation();
  void endLivewirePropagation();

  void modifyUsingLivewire(int, int);

  void curveMousePressEvent(QMouseEvent*);
  void graphcutMousePressEvent(QMouseEvent*);
  void curveMouseMoveEvent(QMouseEvent*);
  void graphcutMouseMoveEvent(QMouseEvent*);

  CurveGroup* getCg();
};


#endif
