#include <qpainter.h>
#include <qcolor.h>
#include "imp_gui.h"



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

static void draw_arrow_left(QPainter *painter, int x, int y,
			    int width, int height) {

  // default arrow shape with x = y = 0, width = 100, & height = 100
  static const QCOORD arrow_shape[9][2] = {{-50,-20},{-10,-50},{-10,-35},{50,-35},
					   {50,50},{20,50},{20,-5},
					   {-10,-5},{-10,10}};

  QCOORD arrow[9][2];
  
  vector2d_scale(arrow, (QCOORD (*)[2])arrow_shape, 9, 0, 0, width, height);
  vector2d_shift(arrow, arrow, 9, x, y);

  painter->drawPolygon(QPointArray(9, (QCOORD *) arrow));
}

static void draw_arrow_right(QPainter *painter, int x, int y,
			     int width, int height) {

  // default arrow shape with x = y = 0, width = 100, & height = 100
  static const QCOORD arrow_shape[9][2] = {{50,-20},{10,-50},{10,-35},{-50,-35},
					   {-50,50},{-20,50},{-20,-5},
					   {10,-5},{10,10}};

  QCOORD arrow[9][2];
  
  vector2d_scale(arrow, (QCOORD (*)[2])arrow_shape, 9, 0, 0, width, height);
  vector2d_shift(arrow, arrow, 9, x, y);

  painter->drawPolygon(QPointArray(9, (QCOORD *) arrow));
}

static void draw_text(QPainter *p, char *s, int y_top) {

  int dw, dh, x0, y0, pos, more;
  char buf[128];

  printf("draw_text(%s)\n", s);

  y0 = y_top;
  more = 1;
  while (more) {
    pos = strcspn(s, "\n");
    strncpy(buf, s, pos);
    buf[pos] = '\0';
    if (strlen(s+pos) <= 1)
      more = 0;
    dw = p->fontMetrics().boundingRect(buf).width();
    dh = p->fontMetrics().boundingRect(buf).height();
    x0 = 80-dw/2;
    y0 = y0+10+dh;
    printf("drawing line at (%d,%d): %s\n", x0, y0, buf);
    p->drawText(x0, y0, buf);
    if (more) {
      s += pos+1;
      if (*s == ' ')
	s++;
    }
  }
}

static void magnify(QPixmap *pixmap) {

  QWMatrix m;
  QPixmap tmp;

  m.scale(1.5, 1.5);
  tmp = pixmap->xForm(m);
  bitBlt(pixmap, 0, 0, &tmp, 0, 0, -1, -1 );
}

/*
 * End Graphics Utility Functions
 **********************************/


/************************
 * Class ArrowDisplay
 */

ArrowDisplay::ArrowDisplay(QWidget *parent = NULL, const char *name = NULL) :
  QWidget(parent, name) {

  setFixedSize(240, 220);
  setBackgroundMode(NoBackground);
  pixmap = new QPixmap(width(), height());
}

ArrowDisplay::~ArrowDisplay() { }

void ArrowDisplay::paintEvent(QPaintEvent *ev) {

  double arrow_angle;
  QPainter painter(this);
  QPainter pixpainter(pixmap);

  arrow_angle = M_PI/2.0 + arrow_theta;
  ev = NULL;

  pixpainter.eraseRect(0, 0, width(), height());
  pixpainter.setPen(QPen(red, 5));
  pixpainter.setBrush(red);

  if (arrow_type == CARMEN_IMP_GUI_ARROW_STRAIGHT)
    draw_arrow(&pixpainter, 120, 120, arrow_angle, 180, 120);
  else if (arrow_type == CARMEN_IMP_GUI_ARROW_LEFT)
    draw_arrow_left(&pixpainter, 120, 120, 160, 160);
  else if (arrow_type == CARMEN_IMP_GUI_ARROW_RIGHT)
    draw_arrow_right(&pixpainter, 120, 120, 160, 160);

  painter.drawPixmap(0, 0, *pixmap);
}

/*
 * End Class ArrowDisplay
 ***************************/


/*************************
 * Class MainMenuDisplayCanvas
 */

MainMenuDisplayCanvas::MainMenuDisplayCanvas(QWidget *parent = NULL, const char *name = NULL) :
  QWidget(parent, name) {

  setFixedSize(200, 120);
  pixmap = new QPixmap(width(), height());
}

MainMenuDisplayCanvas::~MainMenuDisplayCanvas() { }

void MainMenuDisplayCanvas::paintEvent(QPaintEvent *ev) {

  //int w, fsize;
  QPainter painter(this);
  QPainter pixpainter(pixmap);

  ev = NULL;

  if (text == NULL)
    return;

  pixpainter.eraseRect(0, 0, width(), height());
  pixpainter.setPen(black);
  pixpainter.setFont(QFont("times", 24));
  draw_text(&pixpainter, text, 0);
  pixpainter.end();
  magnify(pixmap);

  painter.drawPixmap(0, 0, *pixmap);
}

/*
 * End Class MainMenuDisplayCanvas
 ************************************/


/*************************
 * Class MainMenuDisplay
 */

MainMenuDisplay::MainMenuDisplay(ImpGUI *gui, QWidget* parent = NULL, const char* name = NULL) :
  GuiNav(gui, parent, name) {

  QVBoxLayout *vbox = new QVBoxLayout(this);
  canvas = new MainMenuDisplayCanvas(this);
  QPushButton *yes_button = new QPushButton("Yes", this);
  QPushButton *no_button = new QPushButton("No", this);

  connect(yes_button, SIGNAL(clicked()), this, SLOT(yesButtonHandler()));
  connect(no_button, SIGNAL(clicked()), this, SLOT(noButtonHandler()));

  vbox->addWidget(canvas);
  vbox->addWidget(yes_button);
  vbox->addSpacing(10);
  vbox->addWidget(no_button);
  vbox->addSpacing(10);
}

MainMenuDisplay::~MainMenuDisplay() { }

void MainMenuDisplay::yesButtonHandler() {

  carmen_imp_gui_answer(1);
}

void MainMenuDisplay::noButtonHandler() {

  carmen_imp_gui_answer(0);
}

/*
 * End Class MainMenuDisplay
 ***************************/


/*************************
 * Class GoingToDisplay
 */

GoingToDisplay::GoingToDisplay(ImpGUI *gui, QWidget* parent = NULL, const char* name = NULL) :
  GuiNav(gui, parent, name) {

  QVBoxLayout *vbox = new QVBoxLayout(this);
  arrow_display = new ArrowDisplay(this);

  vbox->addWidget(arrow_display);
}

GoingToDisplay::~GoingToDisplay() { }

/*
 * End Class GoingToDisplay
 ***************************/


/*************************
 * Class ArrivalDisplayCanvas
 */

ArrivalDisplayCanvas::ArrivalDisplayCanvas(QWidget* parent = NULL, const char* name = NULL) :
  QWidget(parent, name) {

  setFixedSize(240, 300);
  pixmap = new QPixmap(width(), height());
}

ArrivalDisplayCanvas::~ArrivalDisplayCanvas() { }

void ArrivalDisplayCanvas::paintEvent(QPaintEvent *ev) {

  char *pos;
  int num_lines;
  QPainter painter(this);
  QPainter pixpainter(pixmap);

  printf("painting ArrivalDisplayCanvas\n");

  ev = NULL;

  if (text == NULL)
    return;

  pixpainter.eraseRect(0, 0, width(), height());
  pixpainter.setPen(black);
  pixpainter.setFont(QFont("times", 24));
  
  pos = text;
  for (num_lines = 1; 1; num_lines++) {
    pos = strchr(pos, '\n');
    if (pos == NULL)
      break;
    pos++;
  }
  if (num_lines > 4)
    num_lines = 4;
  
  draw_text(&pixpainter, text, 10+(4-num_lines)*20);
  pixpainter.end();
  magnify(pixmap);

  painter.drawPixmap(0, 0, *pixmap);
}

/*
 * End Class ArrivalDisplayCanvas
 ************************************/


/*************************
 * Class ArrivalDisplay
 */

ArrivalDisplay::ArrivalDisplay(ImpGUI *gui, QWidget* parent = NULL, const char* name = NULL) :
  GuiNav(gui, parent, name) {

  QVBoxLayout *vbox = new QVBoxLayout(this);
  ArrivalDisplayCanvas *canvas = new ArrivalDisplayCanvas(this);

  vbox->addWidget(canvas);
}

ArrivalDisplay::~ArrivalDisplay() { }

/*
 * End Class ArrivalDisplay
 ***************************/


/*************************
 * Class GuiNav
 */

GuiNav::GuiNav(ImpGUI *gui, QWidget* parent = NULL, const char* name = NULL) :
  QWidget(parent, name) {

  this->gui = gui;

  connect(this, SIGNAL(guiNavEvent(int)), gui, SLOT(setDisplay(int)));
}

GuiNav::~GuiNav() { }

/*
 * End Class GuiNav
 ***************************/


/*************************
 * Class MainDisplay
 */

MainDisplay::MainDisplay(ImpGUI *gui, QWidget* parent = NULL, const char* name = NULL) :
  QWidget(parent, name) {

  QVBoxLayout *vbox = new QVBoxLayout(this);  //dbug - may need to change layout

  main_menu_display = new MainMenuDisplay(gui, this);
  going_to_display = new GoingToDisplay(gui, this);
  arrival_display = new ArrivalDisplay(gui, this);

  vbox->addWidget(main_menu_display);
  vbox->addWidget(going_to_display);
  vbox->addWidget(arrival_display);

  going_to_display->hide();
  main_menu_display->hide();

  display = DISPLAY_ARRIVAL;
}

MainDisplay::~MainDisplay() { }

void MainDisplay::setDisplay(int display) {

  /*
  if (display == this->display) {
    repaint();
    return;
  }
  */

  switch(this->display) {
  case DISPLAY_MAIN_MENU: main_menu_display->hide(); break;
  case DISPLAY_GOING_TO: going_to_display->hide(); break;
  case DISPLAY_ARRIVAL: arrival_display->hide(); break;
  }

  this->display = display;

  switch(this->display) {
  case DISPLAY_MAIN_MENU: main_menu_display->show(); /*main_menu_display->repaint();*/ break;
  case DISPLAY_GOING_TO: going_to_display->show(); /*going_to_display->repaint();*/ break;
  case DISPLAY_ARRIVAL: arrival_display->show(); /*arrival_display->repaint();*/ break;
  }  
}

int MainDisplay::getDisplay() {

  return display;
}

/*
 * End Class MainDisplay
 ***************************/


/*************************
 * Class TitleBar
 */

TitleBar::TitleBar(QWidget* parent = NULL, const char* name = NULL) : QWidget(parent, name) {

  setFixedSize(240, 26);
  setFont(QFont("times", 18));
  setPalette(QPalette(QColor(gray), QColor(darkBlue)));
  QVBoxLayout *vbox = new QVBoxLayout(this);
  title = new QLabel(" ", this); //("<center>main menu</center>", this);
  vbox->addWidget(title);
}

TitleBar::~TitleBar() { }

void TitleBar::setTitle(char *title) {

  char buf[128];

  sprintf(buf, "<center>%s</center>", title);

  this->title->setText(buf);
}

/*
 * End Class TitleBar
 ***************************/


/*************************
 * Class ImpGUI
 */

ImpGUI::ImpGUI(QWidget* parent, const char* name)
  : QWidget(parent, name) {

  setPalette(QPalette(QColor(gray), QColor(white)));
  setFont(QFont("times", 36));
  QVBoxLayout *vbox = new QVBoxLayout(this);
  main_display = new MainDisplay(this, this);
  title_bar = new TitleBar(this);

  vbox->addWidget(title_bar);
  vbox->addWidget(main_display);
}

ImpGUI::~ImpGUI() { }

void ImpGUI::setDisplay(int display) {

  main_display->setDisplay(display);

  /********
  switch(main_display->getDisplay()) {
  case DISPLAY_MAIN_MENU: title_bar->setTitle("main menu"); break;
  case DISPLAY_GOING_TO: title_bar->setTitle("going to..."); break;
  case DISPLAY_ARRIVAL: title_bar->setTitle("you are here"); break;
  }
  *********/
}

/*
 * End Class ImpGUI
 ************************/
