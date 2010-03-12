#ifndef IMP_GUI_H
#define IMP_GUI_H

#include <qwidget.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <carmen/carmen.h>
#include <carmen/imp_gui_interface.h>


extern char *text;
extern double arrow_theta;
extern int arrow_type;

class ImpGUI ;

class GuiNav : public QWidget {
  Q_OBJECT

 public:
  GuiNav(ImpGUI *gui, QWidget* parent = NULL, const char* name = NULL);
  ~GuiNav();

 signals:
  void guiNavEvent(int display);

 private:
  ImpGUI *gui;
};

class MainMenuDisplayCanvas : public QWidget {
  Q_OBJECT

 public:
  MainMenuDisplayCanvas(QWidget *parent = NULL, const char *name = NULL);
  ~MainMenuDisplayCanvas();

 protected:
  void paintEvent(QPaintEvent *ev);

 private:
  QPixmap *pixmap;
};

class MainMenuDisplay : public GuiNav {
  Q_OBJECT

 public:
  MainMenuDisplayCanvas *canvas;
  

  MainMenuDisplay(ImpGUI *gui, QWidget* parent = NULL, const char* name = NULL);
  ~MainMenuDisplay();

 private slots:
  void yesButtonHandler();
  void noButtonHandler();
};

class ArrowDisplay : public QWidget {
  Q_OBJECT

 public:
  ArrowDisplay(QWidget* parent = NULL, const char* name = NULL);
  ~ArrowDisplay();

 protected:
  void paintEvent(QPaintEvent *ev);

 private:
  QPixmap *pixmap;
};

class GoingToDisplay : public GuiNav {
  Q_OBJECT

 public:
  ArrowDisplay *arrow_display;

  GoingToDisplay(ImpGUI *gui, QWidget* parent = NULL, const char* name = NULL);
  ~GoingToDisplay();
};

class ArrivalDisplayCanvas : public QWidget {
  Q_OBJECT

 public:
  ArrivalDisplayCanvas(QWidget* parent = NULL, const char* name = NULL);
  ~ArrivalDisplayCanvas();

 protected:
  void paintEvent(QPaintEvent *ev);

 private:
  QPixmap *pixmap;
};

class ArrivalDisplay : public GuiNav {
  Q_OBJECT

 public:
  ArrivalDisplay(ImpGUI *gui, QWidget* parent = NULL, const char* name = NULL);
  ~ArrivalDisplay();
};

class MainDisplay : public QWidget {
  Q_OBJECT

#define DISPLAY_MAIN_MENU   1
#define DISPLAY_GOING_TO    2
#define DISPLAY_ARRIVAL     3

 public:
  MainMenuDisplay *main_menu_display;
  GoingToDisplay *going_to_display;
  ArrivalDisplay *arrival_display;

  MainDisplay(ImpGUI *gui, QWidget* parent = NULL, const char* name = NULL);
  ~MainDisplay();
  void setDisplay(int display);
  int getDisplay();

 private:
  int display;
};

class TitleBar : public QWidget {
  Q_OBJECT

 public:
  TitleBar(QWidget* parent = NULL, const char* name = NULL);
  ~TitleBar();
  void setTitle(char *title);

 private:
  QLabel *title;
};

class ImpGUI : public QWidget {
  Q_OBJECT

 public:
  MainDisplay *main_display;
  TitleBar *title_bar;

  ImpGUI(QWidget* parent = NULL, const char* name = NULL);
  ~ImpGUI();

 public slots:
  void setDisplay(int display);
};

#endif
