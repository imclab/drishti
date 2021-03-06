#include "livewire.h"

#include <QMessageBox>

LiveWire::LiveWire()
{
  m_image = 0;
  m_grad = 0;
  m_normal = 0;
  m_tmp = 0;
  m_edgeWeight.clear();
  m_cost.clear();
  m_prev.clear();

  m_gradImage = QImage(100, 100, QImage::Format_Indexed8);

  m_width = m_height = 0;

  m_seeds.clear();
  m_seedpos.clear();
  m_poly.clear();
  m_livewire.clear();

  m_propagateLivewire = false;
  m_guessCurve.clear();

  m_gradType = 0;
  m_smoothType = 0;

  m_wtLoG = 0.0;
  m_wtG = 0.7;
  m_wtN = 0.3;

  m_useDynamicTraining = true;
  m_gradCost = 0;

  m_seedMoveMode = false;
  m_activeSeed = -1;
  m_polyA.clear();
  m_polyB.clear();
  m_closed = true;
}

bool LiveWire::seedMoveMode() { return m_seedMoveMode; }

void
LiveWire::setSeedMoveMode(bool b)
{
  m_seedMoveMode = b;
  m_activeSeed = -1;
}

void LiveWire::setGuessCurve(QVector<QPoint> gc) { m_guessCurve = gc; }
// renew guess curve from current livewire polygon
void LiveWire::renewGuessCurve() { m_guessCurve = m_poly; }

bool LiveWire::propagateLivewire() { return m_propagateLivewire; }
void
LiveWire::setPropagateLivewire(bool b)
{
  m_propagateLivewire = b;
  m_guessCurve.clear();
}

bool LiveWire::closed() { return m_closed; }
void
LiveWire::setPolygonToUpdate(QVector<QPoint> pts,
			     QVector<QPoint> seeds,
			     QVector<int> seedpos,
			     bool closed)
{
  m_seedMoveMode = true;
  m_activeSeed = -1;
  m_poly = pts;
  m_seeds = seeds;
  m_seedpos = seedpos;  
  m_closed = closed;
  m_livewire.clear();
}

void LiveWire::setUseDynamicTraining(bool b) { m_useDynamicTraining = b; }

void LiveWire::setWeightLoG(float w) { m_wtLoG = w; }
void LiveWire::setWeightG(float w) { m_wtG = w; }
void LiveWire::setWeightN(float w) { m_wtN = w; }

LiveWire::~LiveWire() { reset(); }

void
LiveWire::resetPoly()
{
  m_seeds.clear();
  m_seedpos.clear();
  m_poly.clear();
  m_livewire.clear();
}

void
LiveWire::reset()
{
  if (m_image) delete [] m_image;
  if (m_grad) delete [] m_grad; 
  if (m_normal) delete [] m_normal;
  if (m_tmp) delete [] m_tmp;
  if (m_gradCost) delete [] m_gradCost;

  m_image = 0;
  m_grad = 0;
  m_normal = 0;
  m_tmp = 0;
  m_gradCost = 0;
  m_edgeWeight.clear();
  m_cost.clear();
  m_prev.clear();

  m_width = m_height = 0;

  m_seeds.clear();
  m_seedpos.clear();
  m_poly.clear();
  m_livewire.clear();

  m_propagateLivewire = false;
  m_guessCurve.clear();

  m_gradCost = new float[256];
  for(int i=0; i<256; i++)
    {
      float gc = 1.0 - (float)i/255.0;
      // scale gc;
      gc = (3*gc*gc - 2*gc*gc*gc);
      m_gradCost[i] = gc;
    }
}

QVector<QPoint> LiveWire::poly() { return m_poly; }
QVector<QPoint> LiveWire::seeds() { return m_seeds; }
QVector<int> LiveWire::seedpos() { return m_seedpos; }
QVector<QPoint> LiveWire::livewire() { return m_livewire; }

void
LiveWire::setImageData(int w, int h, uchar *img)
{
  if (m_width != w || m_height != h)
    {
      if (m_image) delete [] m_image;
      if (m_grad) delete [] m_grad;
      if (m_normal) delete [] m_normal;
      if (m_tmp) delete [] m_tmp;

      m_width = w;
      m_height = h;

      m_image = new uchar[m_width*m_height];
      m_grad = new float[sizeof(float)*m_width*m_height];  
      m_normal = new float[2*sizeof(float)*m_width*m_height];  
      m_tmp = new uchar[4*m_width*m_height];  

      m_edgeWeight.clear();
      m_cost.clear();
      m_prev.clear();
      m_edgeWeight.resize(8*m_width*m_height);
      m_cost.resize(m_width*m_height);
      m_prev.resize(m_width*m_height);
    }

  memcpy(m_image, img, m_width*m_height);
  
  applySmoothing(m_smoothType);
  calculateGradients();
  calculateEdgeWeights();
}

void
LiveWire::mouseReleaseEvent(QMouseEvent *event)
{
  m_activeSeed = -1;
}

bool
LiveWire::mousePressEvent(int xpos, int ypos, QMouseEvent *event)
{
  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;

  if (m_seedMoveMode)
    {
      if (shiftModifier && event->button() == Qt::LeftButton)
	m_activeSeed = insertSeed(xpos, ypos);
      else
	m_activeSeed = getActiveSeed(xpos, ypos);
      return true;
    }

  int button = event->button();
  
//  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
//  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
//  bool altModifier = event->modifiers() & Qt::AltModifier;

  if (button == Qt::LeftButton)
    {
      m_poly += m_livewire;
      m_poly << QPoint(xpos, ypos);

      m_seeds << QPoint(xpos, ypos);
      m_seedpos << m_poly.count()-1;

      //updateGradientCost();

      m_livewire.clear();
      calculateCost(xpos, ypos, 500);

      return true;
    }

  if (button == Qt::RightButton)
    {
      for(int i=m_poly.count()-1; i>=0; i--)
	{
	  if ((m_poly[i]-QPoint(xpos, ypos)).manhattanLength() < 3)
	    {
	      m_poly.remove(i, m_poly.count()-i);

	      //---------------
	      // update seeds to accommodate the removal of points
	      int pc = m_poly.count();
	      for(int j=m_seedpos.count()-1; j>=0; j--)
		{
		  if (m_seedpos[j] < pc)
		    {
		      m_seedpos.remove(j, m_seedpos.count()-j);
		      m_seeds.remove(j, m_seeds.count()-j);
		      m_seedpos << m_poly.count()-1;
		      m_seeds << m_poly[m_poly.count()-1];
		      break;
		    }
		}
	      //---------------

	      m_livewire.clear();
	      calculateCost(xpos, ypos, 500);
	      break;
	    }
	}
      return true;
    }

  return false;
}

bool
LiveWire::mouseMoveEvent(int xpos, int ypos, QMouseEvent *event)
{
//  int button = event->button();
//  
//  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
//  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
//  bool altModifier = event->modifiers() & Qt::AltModifier;

  if (m_seedMoveMode)
    {
      if (event->buttons() == Qt::LeftButton)
	updateLivewireFromSeeds(xpos, ypos);
      return true;
    }

  if (m_poly.count() > 0)
    {
      calculateLivewire(xpos, ypos);
      return true;
    }
  
  return false;
}

void
LiveWire::freeze()
{
  // close the livewire by connecting to the first point
  if (m_poly.count() > 0)
    {
      int xpos = m_poly[0].x();
      int ypos = m_poly[0].y();
      m_livewire.clear();
      calculateLivewire(xpos, ypos);
      m_poly += m_livewire;
    }
}

bool
LiveWire::keyPressEvent(QKeyEvent *event)
{
  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
  bool altModifier = event->modifiers() & Qt::AltModifier;

  if (event->key() == Qt::Key_Escape)
    {
      resetPoly();
      return true;
    }
    
  return false;
}

QImage
LiveWire::gradientImage()
{
  return m_gradImage;
}

void
LiveWire::applySmoothing(int sz)
{
  if (sz == 0) return; // no smoothing

  memcpy(m_tmp, m_image, m_width*m_height);
  gaussBlur_4(m_tmp, m_image, m_width, m_height, sz);
}

void
LiveWire::calculateGradients()
{
//  int sobel_x[3][3] = {{-1,  0,  1},
//			 {-2,  0,  2},
//			 {-1,  0,  1}, };
//
//  int sobel_y[3][3] = {{-1, -2, -1},
//			 { 0,  0,  0},
//			 { 1,  2,  1}, };  

  memset(m_grad, 0, sizeof(float)*m_width*m_height);
  memset(m_normal, 0, 2*sizeof(float)*m_width*m_height);  

  float maxGrad = 0;
  float minGrad = 1000000;

  // central difference
  if (m_gradType == 0)
    {
      for(int i=1; i<m_height-1; i++)
	for(int j=1; j<m_width-1; j++)
	  {	
	    float dx = 0;
	    float dy = 0;
	    {
	      dx += (m_image[(i+1)*m_width+j] -
		     m_image[(i-1)*m_width+j]);
	      
	      dy += (m_image[i*m_width+(j+1)] -
		     m_image[i*m_width+(j-1)]);
	    }
	    
	    float grd = qSqrt(dx*dx + dy*dy);

	    // calculate normal cost
	    if (qAbs(grd) > 0)
	      {
		dx /= grd;
		dy /= grd;
		m_normal[2*(i*m_width+j)] = dy;
		m_normal[2*(i*m_width+j)+1] = -dx;
	      }	    

	    maxGrad = qMax(maxGrad, grd);
	    minGrad = qMin(minGrad, grd);
	    m_grad[i*m_width+j] = grd;
	  }
    }
      

  // sobel
  if (m_gradType == 1)
    {
      for(int i=1; i<m_height-1; i++)
	for(int j=1; j<m_width-1; j++)
	  {	
	    float dx = 0;
	    float dy = 0;
	    {
	      dx += (m_image[(i+1)*m_width+(j-1)] -
		     m_image[(i-1)*m_width+(j-1)]);
	      dx += 2*(m_image[(i+1)*m_width+j] -
		       m_image[(i-1)*m_width+j]);
	      dx += (m_image[(i+1)*m_width+(j+1)] -
		     m_image[(i-1)*m_width+(j+1)]);
	      
	      dy += (m_image[(i+1)*m_width+(j+1)] -
		     m_image[(i+1)*m_width+(j-1)]);
	      dy += 2*(m_image[i*m_width+(j+1)] -
		       m_image[i*m_width+(j-1)]);
	      dy += (m_image[(i-1)*m_width+(j+1)] -
		     m_image[(i-1)*m_width+(j-1)]);
	    }
	    
	    float grd = qSqrt(dx*dx + dy*dy);

	    // calculate normal cost
	    if (qAbs(grd) > 0)
	      {
		dx /= grd;
		dy /= grd;
		m_normal[2*(i*m_width+j)] = dy;
		m_normal[2*(i*m_width+j)+1] = -dx;
	      }	    

	    maxGrad = qMax(maxGrad, grd);
	    minGrad = qMin(minGrad, grd);
	    m_grad[i*m_width+j] = grd;
	  }
    }

  if (maxGrad > minGrad)
    {
      for(int i=0; i<m_width*m_height; i++)
	m_grad[i] = (m_grad[i]-minGrad)/(maxGrad-minGrad);
  

      for(int i=0; i<m_width*m_height; i++)
	{
	  int gidx = 255*(1-m_grad[i]);
	  int c = 200*m_gradCost[gidx];
	  m_tmp[4*i+0] = c;
	  m_tmp[4*i+1] = 0;
	  m_tmp[4*i+2] = c/2;
	  m_tmp[4*i+3] = c;
	}
    }

  m_gradImage = QImage(m_tmp,
		       m_width,
		       m_height,
		       QImage::Format_ARGB32);
}   

void
LiveWire::calculateEdgeWeights()
{  
  for(int i=0; i<m_edgeWeight.count(); i++)
    m_edgeWeight[i] = 10000000;

  for(int i=0; i<m_height; i++)
    for(int j=0; j<m_width; j++)
      {
	int midx = i*m_width+j;
	for(int a=-1; a<=1; a++)
	  for(int b=-1; b<=1; b++)
	    {	      
	      int ia = i+a;
	      int jb = j+b;
	      if (ia < 0 || ia >= m_height ||
		  jb < 0 || jb >= m_width)
		{ }
	      else
		{
		  int idx = (a+1)*3+(b+1);
		  // 0  1  2
		  // 3  4  5
		  // 6  7  8
		  float scl = 1;
		  if (idx%2 == 0) scl = 1.414; // scale for diagonal links

		  if (idx != 4)
		    {
		      if (idx > 4) idx--;
		      // 0  1  2         0  1  2
		      // 3  4  5   ==>   3     4
		      // 6  7  8         5  6  7
		      m_edgeWeight[8*midx + idx] = scl;
		    }
		}
	    }
      }
}

void
LiveWire::calculateCost(int wpos, int hpos, int boxSize)
{
  if (wpos < 0 || wpos >= m_width ||
      hpos < 0 || hpos >= m_height)
    return;

  for(int i=0; i<m_cost.count(); i++)
    m_cost[i] = 10000000.0;
  
  for(int i=0; i<m_prev.count(); i++)
    m_prev[i] = QPoint(-1, -1);

  // used as priority queue
  QMultiMap<float, QPoint> qmap;

  bool *visited = new bool[m_width*m_height];
  memset(visited, 0, m_width*m_height);

  int x = hpos;
  int y = wpos;
  m_cost[x*m_width + y] = 0;
  qmap.insert(0, QPoint(x, y));

  float pi23 = 2.0/(3.0*3.1415926535);
  
  while(qmap.count() > 0)
    {
      float key = qmap.firstKey();
      QList<QPoint> qpr = qmap.values(key);
      float dcost = key;
      int x = qpr[0].x();
      int y = qpr[0].y();
      qmap.remove(key, qpr[0]);
      if (qpr.count() == 1)
	qmap.remove(key);

      int midx = x*m_width+y;

      visited[midx] = true;      
      
      // visit all neighbours
      for(int a=-1; a<=1; a++)
	for(int b=-1; b<=1; b++)
	  {
	    if (x+a < hpos-boxSize || x+a >= hpos+boxSize ||
		y+b < wpos-boxSize || y+b >= wpos+boxSize ||
		x+a < 0 || x+a >= m_height ||
		y+b < 0 || y+b >= m_width)
	      {}
	    else
	      {
		int idx = (a+1)*3+(b+1);
		// 0  1  2
		// 3  4  5
		// 6  7  8
		if (idx != 4 && !visited[(x+a)*m_width+(y+b)])
		  {
		    if (idx > 4) idx--;
		    // 0  1  2         0  1  2
		    // 3  4  5   ==>   3     4
		    // 6  7  8	       5  6  7
		    float lx = (float)a/qSqrt(a*a+b*b);
		    float ly = (float)b/qSqrt(a*a+b*b);
		    float dp = (lx*m_normal[2*midx] +
				ly*m_normal[2*midx+1]);
		    float dq = (lx*m_normal[2*((x+a)*m_width+(y+b))] +
				ly*m_normal[2*((x+a)*m_width+(y+b))+1]);
		    float normalCost = pi23*(qAcos(dp)+qAcos(dq));

		    normalCost *= m_wtN*normalCost;
		    float gradCost = m_wtG*(1.0-m_grad[(x+a)*m_width+(y+b)]);
		    if (m_useDynamicTraining) // take value from gradCost
		      gradCost = m_wtG*m_gradCost[(int)(255*m_grad[(x+a)*m_width+(y+b)])];
		    float ewt = m_edgeWeight.at(8*midx + idx);
		    float newcost = dcost + ewt*(gradCost+normalCost);
		    float oldcost = m_cost.at((x+a)*m_width+(y+b)); 
		    if (newcost < oldcost)
		      {
			m_cost[(x+a)*m_width+(y+b)] = newcost;
			m_prev[(x+a)*m_width+(y+b)] = QPoint(x, y);
			
			qmap.insert(newcost, QPoint(x+a, y+b));
		      }
		  }
	      }
	  }
      
    }

  delete [] visited;
}


void
LiveWire::calculateLivewire(int wpos, int hpos)
{
  if (m_seedMoveMode && m_activeSeed < 0)
    return;

  if (wpos < 0 || wpos >= m_width ||
      hpos < 0 || hpos >= m_height)
    return;

  int x = hpos;
  int y = wpos;
  QVector<QPoint> pts;
  while(x > -1)
    {
      pts << QPoint(y, x);
      int idx = x*m_width+y;
      x = m_prev[idx].x();
      y = m_prev[idx].y();
    }

  m_livewire.clear();
  int pc = pts.count();
  for(int i=0; i<pc; i++)
    m_livewire << pts[pc-1-i];
}

// taken from http://blog.ivank.net/fastest-gaussian-blur.html
QVector<int>
LiveWire::boxesForGauss(float sigma, int n)  // standard deviation, number of boxes
{
  float wIdeal = qSqrt((12*sigma*sigma/n)+1);  // Ideal averaging filter width 
  int wl = wIdeal;
  if(wl%2==0) wl--;
  int wu = wl+2;
				
  float mIdeal = (12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
  int m = mIdeal;
  
  QVector<int> sizes;
  for(int i=0; i<n; i++)
    sizes << (i<m ? wl : wu);

  return sizes;
}
void
LiveWire::gaussBlur_4(uchar *scl, uchar *tcl,
		      int w, int h, int r)
{
  QVector<int> bxs = boxesForGauss(r, 3);
  boxBlur_4 (scl, tcl, w, h, (bxs[0]-1)/2);
  boxBlur_4 (tcl, scl, w, h, (bxs[1]-1)/2);
  boxBlur_4 (scl, tcl, w, h, (bxs[2]-1)/2);
}
void
LiveWire::boxBlur_4(uchar *scl, uchar *tcl,
		    int w, int h, int r)

{
  for(int i=0; i<w*h; i++)
    tcl[i] = scl[i];
  boxBlurH_4(tcl, scl, w, h, r);
  boxBlurT_4(scl, tcl, w, h, r);
}
void
LiveWire::boxBlurH_4(uchar *scl, uchar *tcl,
		     int w, int h, int r)
{
  float iarr = 1.0f/(r+r+1);
  for(int i=0; i<h; i++)
    {
      int ti = i*w;
      int li = ti;
      int ri = ti+r;
      int fv = scl[ti];
      int lv = scl[ti+w-1];
      int val = (r+1)*fv;
      for(int j=0; j<r; j++)
	val += scl[ti+j];
      for(int j=0; j<=r; j++)
	{
	  val += scl[ri++] - fv ;
	  tcl[ti++] = val*iarr;
	}
      for(int j=r+1; j<w-r; j++)
	{
	  val += scl[ri++] - scl[li++];
	  tcl[ti++] = val*iarr;
	}
      for(int j=w-r; j<w; j++)
	{
	  val += lv - scl[li++];
	  tcl[ti++] = val*iarr;
	}
  }
}
void
LiveWire::boxBlurT_4(uchar *scl, uchar *tcl,
		     int w, int h, int r)
{
  float iarr = 1.0f/(r+r+1);
  for(int i=0; i<w; i++)
    {
      int ti = i;
      int li = ti;
      int ri = ti+r*w;
      int fv = scl[ti];
      int lv = scl[ti+w*(h-1)];
      int val = (r+1)*fv;
      for(int j=0; j<r; j++)
	val += scl[ti+j*w];
      for(int j=0; j<=r; j++)
	{
	  val += scl[ri] - fv;
	  tcl[ti] = val*iarr;
	  ri+=w; ti+=w;
	}
    for(int j=r+1; j<h-r; j++)
      {
	val += scl[ri] - scl[li];
	tcl[ti] = val*iarr;
	li+=w; ri+=w; ti+=w;
      }
    for(int j=h-r; j<h; j++)
      {
	val += lv - scl[li];
	tcl[ti] = val*iarr;
	li+=w; ti+=w;
      }
  }
}

void
LiveWire::updateGradientCost()
{
  if (m_poly.count() < 32)
    return;

  int ist = qMax(1,m_poly.count()-64);

  float gs[] = {0.1f, 0.2f, 0.4f, 0.2f, 0.1f};

  float tgc[256];  
  memset(tgc, 0, 256*sizeof(float));

  for(int i=ist; i<m_poly.count(); i++)
    {
      QPoint xy = m_poly[i];
      int h = xy.x();
      int w = xy.y();
      int idx = h*m_width + w;
      float gv = m_grad[idx];
      tgc[(int)(255*gv)]++;
    }

  float mgc = 0;
  for(int i=0; i<256; i++)
    mgc = qMax(mgc, tgc[i]);

  for(int i=0; i<256; i++)
    tgc[i] /= mgc;

  for(int i=0; i<256; i++)
    m_gradCost[i] = 1 - tgc[i];

  // smooth the cost
  for(int i=0; i<256; i++)
    {
      float v=0;
      for(int j=-2;j<=2;j++)
	v += gs[j+2]*m_gradCost[qBound(0,i+j,255)];
      m_gradCost[i] = v;
    }

  for(int i=0; i<m_width*m_height; i++)
    {
      int c = 200*m_gradCost[(int)(255*m_grad[i])];
      m_tmp[4*i+0] = 0;
      m_tmp[4*i+1] = c/2;
      m_tmp[4*i+2] = c;
      m_tmp[4*i+3] = c;
    }

  m_gradImage = QImage(m_tmp,
		       m_width,
		       m_height,
		       QImage::Format_ARGB32);

}

void
LiveWire::livewireFromSeeds(QVector<QPoint> oseeds)
{
  resetPoly();
  
  QVector<bool> used;
  used.fill(false, oseeds.count());
  QVector<QPoint> seeds;
  // ----------
  // lineup all the seeds from orthogonal slices
  for(int i=0; i<m_guessCurve.count(); i++)
    {
      QPoint pt = m_guessCurve[i];
      for(int j=0; j<oseeds.count(); j++)
	{
	  if (!used[j])
	    {
	      if ((pt-oseeds[j]).manhattanLength()<5)
		{
		  used[j] = true;
		  seeds << oseeds[j];
		}
	    }
	}
    }
  // ----------
  
  // find nearest least energy positions for all seeds
  for(int i=0; i<seeds.count(); i++)
    {
      int h = seeds[i].x();
      int w = seeds[i].y();
      int xpos = h;
      int ypos = w;
      int midx = h*m_width+w;
      float minCost = 1.0-m_grad[midx]; // currently look at maximum gradient position
      for(int a=-1; a<=1; a++)
	for(int b=-1; b<=1; b++)
	  {
	    int midx = (qBound(0,(h+a), m_height-1)*m_width+
			qBound(0,(w+b),m_width-1)); 
	    float cst = 1.0-m_grad[midx]; // currently look at maximum gradient position
	    if (minCost > cst)
	      {
		minCost = cst;
		// replace seed
		seeds[i] = QPoint(h+a, w+b);
	      }
	  }
    }

  for(int i=0; i<seeds.count(); i++)
    {
      m_poly += m_livewire;
      m_poly << seeds[i];

      m_seeds << seeds[i];
      m_seedpos << m_poly.count()-1;

      m_livewire.clear();

      int sz = 250;
      if (i < seeds.count()-1)
	sz = qMax(qAbs(seeds[i].x()-seeds[i+1].x()),
		  qAbs(seeds[i].y()-seeds[i+1].y()));
      else
	sz = qMax(qAbs(seeds[i].x()-m_seeds[0].x()),
		  qAbs(seeds[i].y()-m_seeds[0].y()));
	
      sz*=1.5; // make a bigger sized cost matrix 

      calculateCost(seeds[i].x(),
		    seeds[i].y(), sz);

      if (i < seeds.count()-1)
	calculateLivewire(seeds[i+1].x(),
			  seeds[i+1].y());
    }
}

void
LiveWire::updateLivewireFromSeeds(int xpos, int ypos)
{
  if (m_activeSeed < 0)
    return;

  int totseeds = m_seedpos.count();
  int ps = (m_activeSeed-1);
  int ns = (m_activeSeed+1)%totseeds;;
  if (ps < 0) ps = totseeds-1;
  
  m_seeds[m_activeSeed] = QPoint(xpos, ypos);

  int sz = 250;
  int sz0 = qMax(qAbs(xpos-m_seeds[ps].x()),
		 qAbs(ypos-m_seeds[ps].y()));
  int sz1 = qMax(qAbs(xpos-m_seeds[ns].x()),
		 qAbs(ypos-m_seeds[ns].y()));
  sz = 2*(sz0+sz1);
  
  m_poly.clear();

  if (m_activeSeed == 0)
    {
      m_livewire.clear();
      calculateCost(xpos, ypos, 1.5*sz1);
      calculateLivewire(m_seeds[ns].x(),
			m_seeds[ns].y());
      int lwlen = m_livewire.count();
      m_poly += m_livewire;
      int offset = m_poly.count()-1-m_seedpos[1];
      m_poly += m_polyB;
      for(int i=ns; i<m_seedpos.count(); i++)
	m_seedpos[i] += offset;
      
      if (m_closed)
	{
	  m_livewire.clear();
	  calculateCost(m_seeds[ps].x(), m_seeds[ps].y(), 1.5*sz0);
	  calculateLivewire(xpos, ypos);
	  m_poly += m_livewire;
	}
    }
  else
    {
      m_poly += m_polyA;

      m_livewire.clear();
      calculateCost(m_seeds[ps].x(), m_seeds[ps].y(), 1.5*sz0);
      calculateLivewire(xpos, ypos);
      m_poly += m_livewire;
      int offset = m_poly.count()-1-m_seedpos[m_activeSeed];
      for(int i=m_activeSeed; i<m_seedpos.count(); i++)
	m_seedpos[i] += offset;

      if (!m_closed && ns == 0) {}
      else
	{
	  m_livewire.clear();
	  calculateCost(xpos, ypos, 1.5*sz1);
	  calculateLivewire(m_seeds[ns].x(),
			    m_seeds[ns].y());      
	  m_poly += m_livewire;
	}
      if (ns > 0)
	{
	  offset = m_poly.count()-1-m_seedpos[ns];
	  for(int i=ns; i<m_seedpos.count(); i++)
	    m_seedpos[i] += offset;
	  
	  m_poly += m_polyB;
	}
    }

  m_livewire.clear();
}

int
LiveWire::getActiveSeed(int xpos, int ypos)
{
  if (m_activeSeed > -1)
    return m_activeSeed;

  for(int i=0; i<m_seeds.count(); i++)
    {
      if ((m_seeds[i]-QPoint(xpos, ypos)).manhattanLength() < 3)
	{
	  splitPolygon(i);
	  return i;
	}
    }

  m_polyA.clear();
  m_polyB.clear();
  return -1;
}

int
LiveWire::insertSeed(int xpos, int ypos)
{
  int ic = getActiveSeed(xpos, ypos);
  if (ic >= 0)
    return ic;
  
  int sp = -1;
  ic = -1;
  for(int is=1; is<m_seedpos.count(); is++)
    {
      for (int i=m_seedpos[is-1]; i<m_seedpos[is]; i++)
	{
	  if ((m_poly[i]-QPoint(xpos, ypos)).manhattanLength() < 3)
	    {
	      sp = is;
	      ic = i;
	      break;
	    }
	}
    }
  if (sp == -1 || ic == -1)
    {
      if (m_closed)
	{
	  int is = m_seedpos.count()-1;
	  for (int i=m_seedpos[is]; i<m_poly.count(); i++)
	    {
	      if ((m_poly[i]-QPoint(xpos, ypos)).manhattanLength() < 3)
		{
		  sp = is+1;
		  ic = i;
		  break;
		}
	    }
	}

      if (sp == -1 || ic == -1)
	return -1;
    }

  m_seeds.insert(sp, m_poly[ic]);
  m_seedpos.insert(sp, ic);
}

void
LiveWire::splitPolygon(int sp)
{
  m_polyA.clear();
  m_polyB.clear();
  int totseeds = m_seedpos.count();
  int ps = (sp-1);
  int ns = (sp+1)%totseeds;;
  if (ps < 0) ps = totseeds-1;

  if (sp > 1)
    {
      for(int i=0; i<m_seedpos[ps]-1; i++)
	m_polyA << m_poly[i];
    }

  if (ns > 0)
    {
      int pend = m_poly.count();
      if (m_closed && sp == 0)
	pend = m_seedpos[m_seedpos.count()-1];

      for(int i=m_seedpos[ns]+1; i<pend-1; i++)
	m_polyB << m_poly[i];
    }
}

