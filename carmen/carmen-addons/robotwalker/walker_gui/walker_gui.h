#ifndef WALKER_GUI_H
#define WALKER_GUI_H

#include <qwidget.h>
#include <qlayout.h>

class ArrowDisplay : public QWidget {
  Q_OBJECT

 public:
  ArrowDisplay(QWidget* parent = NULL, const char* name = NULL);
  ~ArrowDisplay();
  void setArrowAngle(double angle);

 protected:
  void paintEvent(QPaintEvent *ev);

 private:
  double arrow_angle;
};

class WalkerGui : public QWidget { 
  Q_OBJECT

 public:
  WalkerGui( QWidget* parent = NULL, const char* name = NULL);
  ~WalkerGui();
  void updateArrow(double angle);

 protected:
  ArrowDisplay *arrow_display;
};

#endif
