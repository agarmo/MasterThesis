#include "walker_gui.h"
#include <qpainter.h>
#include <qcolor.h>
#include <math.h>

/**********************************
 * Graphics Utility functions
 */

/*
 * rotate n points of src theta radians about (x,y) and put result in dst.
 */
static void vector2d_rotate(QCOORD dst[][2], QCOORD src[][2], int n,
			    int x, int y, double theta) {

  int i, x2, y2;
  double cos_theta, sin_theta;

  cos_theta = cos(theta);
  sin_theta = sin(theta);

  for (i = 0; i < n; i++) {
    x2 = src[i][0] - x;
    y2 = src[i][1] - y;
    dst[i][0] = (int) (x + cos_theta*x2 - sin_theta*y2 + 0.5);
    dst[i][1] = (int) (y + sin_theta*x2 + cos_theta*y2 + 0.5);
  }
}

static void vector2d_scale(QCOORD dst[][2], QCOORD src[][2],
			   int n, int x, int y,
			   int width_percent, int height_percent) {

  int i, x2, y2;

  for (i = 0; i < n; i++) {
    x2 = src[i][0] - x;
    y2 = src[i][1] - y;
    dst[i][0] = (int) (x + x2*width_percent/100.0 + 0.5);
    dst[i][1] = (int) (y + y2*height_percent/100.0 + 0.5);
  }
}

static void vector2d_shift(QCOORD dst[][2], QCOORD src[][2],
			   int n, int x, int y) {

  int i;

  for (i = 0; i < n; i++) {
    dst[i][0] = src[i][0] + x;
    dst[i][1] = src[i][1] + y;
  }
}

static void draw_arrow(QPainter *painter, int x, int y,
		       double theta, int width, int height) {

  // default arrow shape with x = y = theta = 0, width = 100, & height = 100
  static const QCOORD arrow_shape[7][2] = {{50,0}, {5, -50}, {5, -22},
					{-50, -22}, {-50, 22}, {5, 22},
					{5, 50}};

  QCOORD arrow[7][2];
  
  vector2d_scale(arrow, (QCOORD (*)[2])arrow_shape, 7, 0, 0, width, height);
  vector2d_rotate(arrow, arrow, 7, 0, 0, -theta);
  vector2d_shift(arrow, arrow, 7, x, y);

  painter->drawPolygon(QPointArray(7, (QCOORD *) arrow));
}

ArrowDisplay::ArrowDisplay(QWidget *parent, const char *name)
  : QWidget(parent, name) {

}

ArrowDisplay::~ArrowDisplay() { }

void ArrowDisplay::setArrowAngle(double angle) {

  arrow_angle = angle;
  emit paintEvent(NULL);
}

void ArrowDisplay::paintEvent(QPaintEvent *ev) {

  ev = NULL;

  QPainter painter(this);

  painter.eraseRect(0, 0, width(), height());
  painter.setBrush(QColor(255,0,0));
  draw_arrow(&painter, 120, 160, arrow_angle, 160, 100);
}

WalkerGui::WalkerGui(QWidget* parent, const char* name)
  : QWidget(parent, name) {
 
  QVBoxLayout *vbox = new QVBoxLayout(this);
  
  arrow_display = new ArrowDisplay(this);

  vbox->addWidget(arrow_display);
}

WalkerGui::~WalkerGui() { }

void WalkerGui::updateArrow(double angle) {

  arrow_display->setArrowAngle(angle);
}
