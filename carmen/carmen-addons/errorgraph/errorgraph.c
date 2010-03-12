#include <carmen/carmen_graphics.h>
#include <gsl/gsl_linalg.h>


/* public parameters */
static double srrad = 0.1;  /* shift rotational radius (meters) */
static int srnum = 50;  /* shift rotational number of points plotted */
static int irdisp = 64;  /* inverse rotational displacement */
static int rnum = 50;  /* rotational number of points plotted */
/* end public parameters */

#define NUM_PARAMS 4
static double tmp_srrad;
static int tmp_srnum, tmp_irdisp, tmp_rnum;
static char *srrad_name = "circular_shift_radius";
static char *srnum_name = "circular_shift_num";
static char *irdisp_name = "inverse_rotational_epsilon";
static char *rnum_name = "rotational_num";


#define MAX_NUM_READINGS 1800
#define MAX_SRNUM 3601
#define MAX_RNUM 3601

static carmen_laser_laser_message uscan, scan;
static carmen_localize_globalpos_message robot_pos;
static double robot_x, robot_y, robot_theta;
static double frontlaser_offset, tmp_frontlaser_offset;
static double max_range, tmp_max_range;
static int update = 0, update_pos = 1, display_graphs = 0;
static int computing_serror = 0, computing_rerror = 0;
static double serror[MAX_SRNUM], max_serror, rerror[MAX_RNUM], max_rerror;
static double rcurv, max_rcurv;

#define RCANVAS_STATE_ERROR 0
#define RCANVAS_STATE_CURVATURE 1

static int rcanvas_state = RCANVAS_STATE_ERROR;

static GtkWidget *scanvas, *rcanvas;
static GdkPixmap *spixmap = NULL, *rpixmap = NULL;
static int scanvas_width = 300, scanvas_height = 300;
static int rcanvas_width = 300, rcanvas_height = 300;

static GdkGC *drawing_gc = NULL;
static GdkColor gdkcolor_black;

static char *grfont_name = "-*-grtt-medium-r-*--*-140-*-*-*-*-iso8859-7";
static GdkFont *grfont;

static char char_theta = 0xe8;
static char char_pi = 0xf0;
static char char_df = 0xb0;

static char *slabel = "circular shift error";
static char *rlabel_err = "epsilon rotational error";
static char *rlabel_curv = "epsilon rotational curvature";

static double gratio = 0.8;
static double semax = 10.0;  /* shift error max */
static double remax = 10.0;  /* rotational error max */
static int setdiv = 3;  /* shift error tick division */
static int retdiv = 3;  /* rotational error tick division */
static int sttdiv = 3;  /* shift theta tick division */
static int rttdiv = 3;  /* rotational theta tick division */
static int setnum = 10;  /* shift error tick number */
static int retnum = 10;  /* rotational error tick number */
static int sttnum = 8;  /* shift theta tick number */
static int rttnum = 8;  /* rotational theta tick number */
static int sgpadx = 50;  /* shift graph x-padding (pixels) */
static int sgpady = 30;  /* shift graph y-padding (pixels) */
static int rgpadx = 50;  /* rotational graph x-padding (pixels) */
static int rgpady = 30;  /* rotational graph y-padding (pixels) */
static int setlen1 = 5;  /* shift error sub-tick length (pixels) */
static int setlen2 = 10;  /* shift error super-tick length (pixels) */
static int retlen1 = 5;  /* rotational error sub-tick length (pixels) */
static int retlen2 = 10;  /* rotational error super-tick length (pixels) */
static int sttlen1 = 5;  /* shift theta sub-tick length (pixels) */
static int sttlen2 = 10;  /* shift theta super-tick length (pixels) */
static int rttlen1 = 5;  /* rotational theta sub-tick length (pixels) */
static int rttlen2 = 10;  /* rotational theta super-tick length (pixels) */
static int textpadx = 10;  /* (pixels) */
static int textpady = 10;  /* (pixels) */

static gint draw_graph(gpointer p) {

  int i, j, x_1, y_1, x_2, y_2;
  GtkWidget *canvas;
  GdkFont *font;
  char buf[1024];  

  canvas = GTK_WIDGET(p);
  if (grfont)
    font = grfont;
  else
    font = canvas->style->font;

  gdk_gc_set_foreground(drawing_gc, &gdkcolor_black);

  if (canvas == scanvas) {

    gdk_draw_rectangle(spixmap, canvas->style->white_gc, TRUE, 0, 0,
		       scanvas_width, scanvas_height);

    gdk_draw_line(spixmap, drawing_gc, sgpadx, sgpady, sgpadx,
		  scanvas_height - sgpady);
    
    for (i = 1; i <= setnum; i++) {
      x_1 = sgpadx - (setlen2 / 2.0);
      x_2 = sgpadx + (setlen2 / 2.0);
      y_1 = y_2 =
	sgpady + (setnum - i) * (scanvas_height - 2 * sgpady) / setnum;
      gdk_draw_line(spixmap, drawing_gc, x_1, y_1, x_2, y_2);
      for (j = 1; j <= setdiv; j++) {
	x_1 = sgpadx - (setlen1 / 2.0);
	x_2 = sgpadx + (setlen1 / 2.0);
	y_1 += (scanvas_height - 2 * sgpady) / (setnum * (setdiv + 1));
	y_2 = y_1;
	gdk_draw_line(spixmap, drawing_gc, x_1, y_1, x_2, y_2);
      }
    }

    gdk_draw_line(spixmap, drawing_gc, sgpadx, scanvas_height - sgpady,
		  scanvas_width - sgpadx, scanvas_height - sgpady);

    for (i = 1; i <= sttnum; i++) {
      x_1 = x_2 =
	sgpadx + i * (scanvas_width - 2 * sgpadx) / sttnum;
      y_1 = scanvas_height - sgpady - (sttlen2 / 2.0);
      y_2 = scanvas_height - sgpady + (sttlen2 / 2.0);
      gdk_draw_line(spixmap, drawing_gc, x_1, y_1, x_2, y_2);
      for (j = 1; j <= sttdiv; j++) {
	x_1 -= (scanvas_width - 2 * sgpadx) / (sttnum * (sttdiv + 1));
	x_2 = x_1;
	y_1 = scanvas_height - sgpady - (sttlen1 / 2.0);
	y_2 = scanvas_height - sgpady + (sttlen1 / 2.0);
	gdk_draw_line(spixmap, drawing_gc, x_1, y_1, x_2, y_2);
      }
    }

    x_1 = sgpadx - gdk_string_width(font, "E") / 2.0;
    y_1 = sgpady - textpady;
    gdk_draw_string(spixmap, font, drawing_gc, x_1, y_1, "E");

    x_1 = scanvas_width - sgpadx + textpadx;
    y_1 = scanvas_height - sgpady + font->ascent / 2.0;
    if (grfont)
      sprintf(buf, "%c", char_theta);
    else
      sprintf(buf, "theta");
    gdk_draw_string(spixmap, font, drawing_gc, x_1, y_1, buf);

    x_1 = sgpadx - gdk_string_width(font, "0");
    y_1 = scanvas_height - sgpady + font->ascent;
    gdk_draw_string(spixmap, font, drawing_gc, x_1, y_1, "0");

    if (grfont)
      sprintf(buf, "2%c", char_pi);
    else
      sprintf(buf, "360%c", char_df);
    x_1 = scanvas_width - sgpadx - gdk_string_width(font, buf) / 2.0;
    y_1 = scanvas_height - sgpady + textpady + font->ascent;
    gdk_draw_string(spixmap, font, drawing_gc, x_1, y_1, buf);

    if (grfont)
      sprintf(buf, "%c", char_pi);
    else
      sprintf(buf, "180%c", char_df);
    x_1 = scanvas_width / 2.0 - gdk_string_width(font, buf) / 2.0;
    y_1 = scanvas_height - sgpady + textpady + font->ascent;
    gdk_draw_string(spixmap, font, drawing_gc, x_1, y_1, buf);

    sprintf(buf, "%.2f", semax);
    x_1 = sgpadx - textpadx - gdk_string_width(font, buf);
    y_1 = sgpady + font->ascent / 2.0;
    gdk_draw_string(spixmap, font, drawing_gc, x_1, y_1, buf);

    sprintf(buf, "%.2f", semax / 2.0);
    x_1 = sgpadx - textpadx - gdk_string_width(font, buf);
    y_1 = (scanvas_height + font->ascent) / 2.0;
    gdk_draw_string(spixmap, font, drawing_gc, x_1, y_1, buf);

    x_1 = scanvas_width - sgpadx / 2.0 - gdk_string_width(font, slabel);
    y_1 = sgpady;
    gdk_draw_string(spixmap, font, drawing_gc, x_1, y_1, slabel);

    /* now for the real graphing */

    if (display_graphs && !computing_serror) {
      x_2 = sgpadx;
      y_2 = scanvas_height - sgpady - (serror[0] / (double) semax) *
	(double) (scanvas_height - 2 * sgpady);
      for(i = 1; i < srnum; i++) {
	x_1 = x_2;
	y_1 = y_2;
	x_2 = sgpadx + (i / (double) (srnum - 1)) * (double) (scanvas_width -
							      2 * sgpadx);
	y_2 = scanvas_height - sgpady - (serror[i] / (double) semax) *
	  (double) (scanvas_height - 2 * sgpady);
	gdk_draw_line(spixmap, drawing_gc, x_1, y_1, x_2, y_2);
      }
    }

    gdk_draw_pixmap(canvas->window,
		    canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		    spixmap, 0, 0, 0, 0, scanvas_width, scanvas_height);
  }
  else {

    gdk_draw_rectangle(rpixmap, canvas->style->white_gc, TRUE, 0, 0,
		       rcanvas_width, rcanvas_height);

    gdk_draw_line(rpixmap, drawing_gc, rgpadx, rgpady, rgpadx,
		  rcanvas_height - rgpady);
    
    for (i = 1; i <= retnum; i++) {
      x_1 = rgpadx - (retlen2 / 2.0);
      x_2 = rgpadx + (retlen2 / 2.0);
      y_1 = y_2 =
	rgpady + (retnum - i) * (rcanvas_height - 2 * rgpady) / retnum;
      gdk_draw_line(rpixmap, drawing_gc, x_1, y_1, x_2, y_2);
      for (j = 1; j <= retdiv; j++) {
	x_1 = rgpadx - (retlen1 / 2.0);
	x_2 = rgpadx + (retlen1 / 2.0);
	y_1 += (rcanvas_height - 2 * rgpady) / (retnum * (retdiv + 1));
	y_2 = y_1;
	gdk_draw_line(rpixmap, drawing_gc, x_1, y_1, x_2, y_2);
      }
    }

    gdk_draw_line(rpixmap, drawing_gc, rgpadx, rcanvas_height - rgpady,
		  rcanvas_width - rgpadx, rcanvas_height - rgpady);

    for (i = 1; i <= rttnum; i++) {
      x_1 = x_2 =
	rgpadx + i * (rcanvas_width - 2 * rgpadx) / rttnum;
      y_1 = rcanvas_height - rgpady - (rttlen2 / 2.0);
      y_2 = rcanvas_height - rgpady + (rttlen2 / 2.0);
      gdk_draw_line(rpixmap, drawing_gc, x_1, y_1, x_2, y_2);
      for (j = 1; j <= rttdiv; j++) {
	x_1 -= (rcanvas_width - 2 * rgpadx) / (rttnum * (rttdiv + 1));
	x_2 = x_1;
	y_1 = rcanvas_height - rgpady - (rttlen1 / 2.0);
	y_2 = rcanvas_height - rgpady + (rttlen1 / 2.0);
	gdk_draw_line(rpixmap, drawing_gc, x_1, y_1, x_2, y_2);
      }
    }    

    x_1 = rgpadx - textpadx - gdk_string_width(font, "0");
    y_1 = rcanvas_height - rgpady + 5;
    gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, "0");

    x_1 = rcanvas_width - rgpadx + textpadx;
    y_1 = rcanvas_height - rgpady + font->ascent / 2.0;
    if (grfont)
      sprintf(buf, "%c", char_theta);
    else
      sprintf(buf, "theta");
    gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, buf);
    
    if (grfont)
      sprintf(buf, "%c/%d", char_pi, irdisp);
    else
      sprintf(buf, "%.1f%c", 180 / (double) irdisp, char_df);
    x_1 = rcanvas_width - rgpadx - gdk_string_width(font, buf) / 2.0;
    y_1 = rcanvas_height - rgpady + textpady + font->ascent;
    gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, buf);
    
    x_1 = rcanvas_width / 2.0 - gdk_string_width(font, "0") / 2.0;
    y_1 = rcanvas_height - rgpady + textpady + font->ascent;
    gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, "0");
    
    if (grfont)
      sprintf(buf, "-%c/%d", char_pi, irdisp);
    else
      sprintf(buf, "-%.1f%c", 180 / (double) irdisp, char_df);
    x_1 = rgpadx - gdk_string_width(font, buf) / 2.0 + 3;
    y_1 = rcanvas_height - rgpady + textpady + font->ascent;
    gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, buf);
    
    gdk_draw_line(rpixmap, drawing_gc, rgpadx, rcanvas_height - rgpady,
		  rgpadx - 15, rcanvas_height - rgpady + 15);
    
    if (rcanvas_state == RCANVAS_STATE_CURVATURE) {
      x_1 = rgpadx - gdk_string_width(font, "C") / 2.0;
      y_1 = rgpady - textpady;
      gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, "C");

      sprintf(buf, "%.0f", max_rcurv);
      x_1 = rgpadx - textpadx - gdk_string_width(font, buf);
      y_1 = rgpady + font->ascent / 2.0;
      gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, buf);

      sprintf(buf, "%.0f", max_rcurv / 2.0);
      x_1 = rgpadx - textpadx - gdk_string_width(font, buf);
      y_1 = (rcanvas_height + font->ascent) / 2.0;
      gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, buf);

      x_1 = rcanvas_width - rgpadx / 2.0 - gdk_string_width(font, rlabel_curv);
      y_1 = rgpady;
      gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, rlabel_curv);

      /* now for the real graphing */

      if (display_graphs && !computing_rerror) {
	y_1 = rcanvas_height - rgpady - (rcurv / max_rcurv) *
	      (rcanvas_height - 2.0 * rgpady);
	gdk_draw_line(rpixmap, drawing_gc, rgpadx, y_1,
		      rcanvas_width - rgpadx, y_1);
      }
    }
    else {
      x_1 = rgpadx - gdk_string_width(font, "E") / 2.0;
      y_1 = rgpady - textpady;
      gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, "E");

      sprintf(buf, "%.2f", remax);
      x_1 = rgpadx - textpadx - gdk_string_width(font, buf);
      y_1 = rgpady + font->ascent / 2.0;
      gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, buf);

      sprintf(buf, "%.2f", remax / 2.0);
      x_1 = rgpadx - textpadx - gdk_string_width(font, buf);
      y_1 = (rcanvas_height + font->ascent) / 2.0;
      gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, buf);

      x_1 = rcanvas_width - rgpadx / 2.0 - gdk_string_width(font, rlabel_err);
      y_1 = rgpady;
      gdk_draw_string(rpixmap, font, drawing_gc, x_1, y_1, rlabel_err);

      /* now for the real graphing */

      if (display_graphs && !computing_rerror) {
	x_2 = rgpadx;
	y_2 = rcanvas_height - rgpady - (rerror[0] / (double) remax) *
	  (double) (rcanvas_height - 2 * rgpady);
	for(i = 1; i < rnum; i++) {
	  x_1 = x_2;
	  y_1 = y_2;
	  x_2 = rgpadx + (i / (double) (rnum - 1)) * (double) (rcanvas_width -
							       2 * rgpadx);
	  y_2 = rcanvas_height - rgpady - (rerror[i] / (double) remax) *
	    (double) (rcanvas_height - 2 * rgpady);
	  gdk_draw_line(rpixmap, drawing_gc, x_1, y_1, x_2, y_2);
	}
      }
    }

    gdk_draw_pixmap(canvas->window,
		    canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		    rpixmap, 0, 0, 0, 0, rcanvas_width, rcanvas_height);
  }

  return FALSE;
}

static double compute_error_with_shift(double x_shift, double y_shift,
				       double theta_shift) {

  int i, j, scan_mask[MAX_NUM_READINGS];
  double x, y, theta, laser_x, laser_y, x2, y2, theta2;
  double error, min_error, total_error;
  /*
  printf("\n-----------------------------\n\n");
  printf("x_shift = %f, y_shift = %f, theta_shift = %f\n",
	 x_shift, y_shift, theta_shift);
  printf("\n-----------------------------\n\n");
  */
  total_error = 0.0;

  laser_x = robot_x + frontlaser_offset * cos(robot_theta);
  laser_y = robot_y + frontlaser_offset * sin(robot_theta);

  for (i = 0; i < scan.num_readings; i++)
    scan_mask[i] = (scan.range[i] < max_range);

  for (i = 0; i < scan.num_readings; i++) {
    if (scan_mask[i] == 0)
      continue;
    theta = robot_theta - M_PI_2 +
      (i / (double) scan.num_readings) * M_PI;
    x = laser_x + scan.range[i] * cos(theta);
    y = laser_y + scan.range[i] * sin(theta);
    /*
    printf("theta (degrees) = %.1f, x = %.1f, y = %.1f\n",
	   carmen_radians_to_degrees(theta), x, y);
    */
    min_error = (max_range * max_range) / 2.0;
    for (j = 0; j < scan.num_readings; j++) {
      if (scan_mask[j] == 0)
	continue;
      theta2 = theta_shift + robot_theta - M_PI_2 +
	(j / (double) scan.num_readings) * M_PI;
      x2 = laser_x + x_shift + scan.range[j] * cos(theta2);
      y2 = laser_y + y_shift + scan.range[j] * sin(theta2);
      /*
      printf("theta2 (degrees) = %.1f, x2 = %.1f, y2 = %.1f\n",
	     carmen_radians_to_degrees(theta2), x2, y2);
      */
      error = ((x2 - x) * (x2 - x) + (y2 - y) * (y2 - y)) / 2.0;
      /*
      printf("x2 - x = %f, y2 - y = %f, error = %f, min_error = %f\n",
	     x2 - x, y2 - y, error, min_error);
      */
      if (error < min_error)
	min_error = error;
    }
    total_error += min_error;
  }
  /*
  printf("total_error = %f\n", total_error);
  */
  return total_error;
}

static gint sgraph_iteration(gpointer p) {

  double theta;
  int i = *((int *) p);

  if (i < srnum) {
    theta = 2.0 * M_PI * i / (double) (srnum - 1);
    serror[i] =
      compute_error_with_shift(srrad * cos(theta), srrad * sin(theta), 0.0);
    if (serror[i] > max_serror)
      max_serror = serror[i];
    (*((int *) p))++;
    return TRUE;
  }
  else {
    free(p);
    if (max_serror < 5.0) {
      if (max_serror < 0.5)
	semax = 0.1 * ceil(10.0 * max_serror / gratio);
      else
	semax = ceil(max_serror / gratio);
    }
    else
      semax = 10.0 * ceil(max_serror / (10.0 * gratio));
    computing_serror = 0;
    draw_graph((gpointer) scanvas);
    if (!computing_rerror)
      update_pos = 1;
    return FALSE;
  }
}

static void sgraph() {

  int *iptr;

  computing_serror = 1;
  srrad = tmp_srrad;
  srnum = tmp_srnum;
  max_serror = 0.0;
  iptr = (int *) calloc(1, sizeof(int));
  carmen_test_alloc(iptr);
  *iptr = 0;

  gtk_idle_add(sgraph_iteration, (gpointer) iptr);
}

static void compute_rcurv() {

  gsl_vector *s, *b, *x, *tmp;
  gsl_matrix *A, *V, *TMP;
  int i;
  double x_coord;

  s = gsl_vector_alloc(3);
  x = gsl_vector_alloc(3);
  tmp = gsl_vector_alloc(3);
  b = gsl_vector_alloc(rnum);
  A = gsl_matrix_alloc(rnum, 3);
  V = gsl_matrix_alloc(3, 3);
  TMP = gsl_matrix_alloc(3, 3);

  for (i = 0; i < rnum; i++) {
    x_coord = (i - rnum / 2.0) * 2.0 * M_PI / (irdisp * (double) rnum);
    gsl_matrix_set(A, i, 0, 1.0);
    gsl_matrix_set(A, i, 1, x_coord);
    gsl_matrix_set(A, i, 2, x_coord * x_coord);
    gsl_vector_set(b, i, rerror[i]);
  }

  gsl_linalg_SV_decomp_mod(A, TMP, V, s, tmp);
  gsl_linalg_SV_solve(A, V, s, b, x);

  rcurv = gsl_vector_get(x, 2);

  if (100 * ceil(rcurv / (100.0 * gratio)) > max_rcurv)
    max_rcurv = 100 * ceil(rcurv / (100.0 * gratio));

  gsl_vector_free(s);
  gsl_vector_free(x);
  gsl_vector_free(b);
  gsl_vector_free(tmp);
  gsl_matrix_free(A);
  gsl_matrix_free(V);
  gsl_matrix_free(TMP);
}

static gint rgraph_iteration(gpointer p) {

  double theta;
  int i = *((int *) p);

  if (i < rnum) {
    theta = -M_PI / (double) irdisp + 2.0 * M_PI * i / ((rnum - 1) *
							(double) irdisp);
    rerror[i] =
      compute_error_with_shift(0.0, 0.0, theta);
    if (rerror[i] > max_rerror)
      max_rerror = rerror[i];
    (*((int *) p))++;
    return TRUE;
  }
  else {
    free(p);
    if (rcanvas_state == RCANVAS_STATE_CURVATURE)
      compute_rcurv();
    else {
      if (max_rerror < 5.0) {
	if (max_rerror < 0.5)
	  remax = 0.1 * ceil(10.0 * max_rerror / gratio);
	else
	  remax = ceil(max_rerror / gratio);
      }
      else
	remax = 10 * ceil(max_rerror / (10.0 * gratio));
    }
    computing_rerror = 0;
    draw_graph((gpointer) rcanvas);
    if (!computing_serror)
      update_pos = 1;
    return FALSE;
  }
}

static void rgraph() {

  int *iptr;

  computing_rerror = 1;
  irdisp = tmp_irdisp;
  rnum = tmp_rnum;
  max_rerror = 0.0;
  iptr = (int *) calloc(1, sizeof(int));
  carmen_test_alloc(iptr);
  *iptr = 0;

  gtk_idle_add(rgraph_iteration, (gpointer) iptr);
}

static void scan_update() {

  if (!update || update_pos)
    return;

  if (computing_serror || computing_rerror)
    return;

  if (uscan.range == NULL) {
    fprintf(stderr, "arrrgghhhh, scan.range is NULL!\n");
    return;
  }

  scan.num_readings = uscan.num_readings;
  memcpy(scan.range, uscan.range, scan.num_readings * sizeof(double));

  if (display_graphs) {
    max_range = tmp_max_range;
    frontlaser_offset = tmp_frontlaser_offset;
    sgraph();
    rgraph();
  }
  else
    update_pos = 1;
}

static void robot_pos_update() {

  if (computing_serror || computing_rerror || !update_pos)
    return;

  robot_x = robot_pos.globalpos.x;
  robot_y = robot_pos.globalpos.y;
  robot_theta = robot_pos.globalpos.theta;
  /*
  printf("robot pose = %.1f, %.1f, %.1f\n", robot_x, robot_y,
	 carmen_radians_to_degrees(robot_theta));
  */
  display_graphs = 1;
  update_pos = 0;
}

static void scan_init() {

  scan.range = calloc(MAX_NUM_READINGS, sizeof(double));
  carmen_test_alloc(scan.range);

  carmen_laser_subscribe_frontlaser_message(&uscan,
					  (carmen_handler_t) scan_update,
					  CARMEN_SUBSCRIBE_LATEST);
  carmen_localize_subscribe_globalpos_message(&robot_pos,
					    (carmen_handler_t) robot_pos_update,
					    CARMEN_SUBSCRIBE_LATEST);
}

static void canvas_expose(GtkWidget *canvas, GdkEventExpose *event) {

  gdk_draw_pixmap(canvas->window,
		  canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		  canvas == scanvas ? spixmap : rpixmap,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);
}

static void canvas_configure(GtkWidget *canvas) {

  int display = (drawing_gc != NULL);

  if (canvas == scanvas) {
    scanvas_width = canvas->allocation.width;
    scanvas_height = canvas->allocation.height;
    if (spixmap)
      gdk_pixmap_unref(spixmap);
    spixmap = gdk_pixmap_new(canvas->window, scanvas_width,
			     scanvas_height, -1);
    gdk_draw_rectangle(spixmap, canvas->style->white_gc, TRUE, 0, 0,
		       scanvas_width, scanvas_height);
    gdk_draw_pixmap(canvas->window,
		    canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		    spixmap, 0, 0, 0, 0, scanvas_width, scanvas_height);
  }
  else {
    rcanvas_width = canvas->allocation.width;
    rcanvas_height = canvas->allocation.height;
    if (rpixmap)
      gdk_pixmap_unref(rpixmap);
    rpixmap = gdk_pixmap_new(canvas->window, rcanvas_width,
			     rcanvas_height, -1);
    gdk_draw_rectangle(rpixmap, canvas->style->white_gc, TRUE, 0, 0,
		       rcanvas_width, rcanvas_height);
    gdk_draw_pixmap(canvas->window,
		    canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		    rpixmap, 0, 0, 0, 0, rcanvas_width, rcanvas_height);
  }

  if (display)
    draw_graph((gpointer) canvas);
}

static void window_destroy(GtkWidget *w __attribute__ ((unused))) {

  gtk_main_quit();
}

static void rcanvas_toggle_state(GtkWidget *w __attribute__ ((unused)),
				 gpointer p __attribute__ ((unused))) {

  rcanvas_state = !rcanvas_state;
}

static void gui_init() {

  GtkWidget *window, *vbox;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, FALSE);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(window_destroy), NULL);

  vbox = gtk_vbox_new(TRUE, 0);

  scanvas = gtk_drawing_area_new();
  rcanvas = gtk_drawing_area_new();

  grfont = gdk_font_load(grfont_name);

  gtk_drawing_area_size(GTK_DRAWING_AREA(scanvas), scanvas_width,
			scanvas_height);
  gtk_drawing_area_size(GTK_DRAWING_AREA(rcanvas), rcanvas_width,
			rcanvas_height);

  gtk_box_pack_start(GTK_BOX(vbox), scanvas, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), rcanvas, TRUE, TRUE, 0);

  gtk_signal_connect(GTK_OBJECT(scanvas), "expose_event",
		     GTK_SIGNAL_FUNC(canvas_expose), NULL);

  gtk_signal_connect(GTK_OBJECT(scanvas), "configure_event",
		     GTK_SIGNAL_FUNC(canvas_configure), NULL);

  gtk_signal_connect(GTK_OBJECT(rcanvas), "expose_event",
		     GTK_SIGNAL_FUNC(canvas_expose), NULL);

  gtk_signal_connect(GTK_OBJECT(rcanvas), "configure_event",
		     GTK_SIGNAL_FUNC(canvas_configure), NULL);

  gtk_signal_connect(GTK_OBJECT(rcanvas), "button_press_event",
		     GTK_SIGNAL_FUNC(rcanvas_toggle_state), NULL);

  gtk_widget_add_events(rcanvas, GDK_BUTTON_PRESS_MASK);

  gtk_container_add(GTK_CONTAINER(window), vbox);

  gtk_widget_show_all(window);

  drawing_gc = gdk_gc_new(scanvas->window);
  gdkcolor_black = carmen_graphics_add_color("black");

  draw_graph((gpointer) scanvas);
  draw_graph((gpointer) rcanvas);

  update = 1;
}

static void params_init(int argc __attribute__ ((unused)),
			char *argv[] __attribute__ ((unused))) {

  int num_items;
  carmen_param_t param_list[NUM_PARAMS];
  int retval;

  num_items = 0;
  carmen_param_set_module("errorgraph");

  retval = carmen_param_get_double(srrad_name, &tmp_srrad);
  if (retval >= 0) {
    srrad = tmp_srrad;
    carmen_param_subscribe_double("errorgraph", srrad_name, &tmp_srrad, NULL);
  }
  else {
    tmp_srrad = srrad;
    param_list[num_items].module = "errorgraph";
    param_list[num_items].variable = srrad_name;
    param_list[num_items].type = CARMEN_PARAM_DOUBLE;
    param_list[num_items].user_variable = &tmp_srrad;
    param_list[num_items].subscribe = 1;
    param_list[num_items].handler = NULL;
    num_items++;
  }

  retval = carmen_param_get_int(srnum_name, &tmp_srnum);
  if (retval >= 0) {
    srnum = tmp_srnum;
    carmen_param_subscribe_int("errorgraph", srnum_name, &tmp_srnum, NULL);
  }
  else {
    tmp_srnum = srnum;
    param_list[num_items].module = "errorgraph";
    param_list[num_items].variable = srnum_name;
    param_list[num_items].type = CARMEN_PARAM_INT;
    param_list[num_items].user_variable = &tmp_srnum;
    param_list[num_items].subscribe = 1;
    param_list[num_items].handler = NULL;
    num_items++;
  }

  retval = carmen_param_get_int(irdisp_name, &tmp_irdisp);
  if (retval >= 0) {
    srnum = tmp_irdisp;
    carmen_param_subscribe_int("errorgraph", irdisp_name, &tmp_irdisp, NULL);
  }
  else {
    tmp_irdisp = irdisp;
    param_list[num_items].module = "errorgraph";
    param_list[num_items].variable = irdisp_name;
    param_list[num_items].type = CARMEN_PARAM_INT;
    param_list[num_items].user_variable = &tmp_irdisp;
    param_list[num_items].subscribe = 1;
    param_list[num_items].handler = NULL;
    num_items++;
  }

  retval = carmen_param_get_int(rnum_name, &tmp_rnum);
  if (retval >= 0) {
    srnum = tmp_rnum;
    carmen_param_subscribe_int("errorgraph", rnum_name, &tmp_rnum, NULL);
  }
  else {
    tmp_rnum = rnum;
    param_list[num_items].module = "errorgraph";
    param_list[num_items].variable = rnum_name;
    param_list[num_items].type = CARMEN_PARAM_INT;
    param_list[num_items].user_variable = &tmp_rnum;
    param_list[num_items].subscribe = 1;
    param_list[num_items].handler = NULL;
    num_items++;
  }
  
  if (num_items > 0)
    carmen_param_install_params(argc, argv, param_list, num_items);
  carmen_param_set_module(NULL);

  retval = 
    carmen_param_get_double("robot_frontlaser_offset", &tmp_frontlaser_offset);
  if (retval < 0)
    fprintf(stderr, "ERROR\n");
  retval = carmen_param_get_double("robot_front_laser_max", &tmp_max_range);
  if (retval < 0)
    fprintf(stderr, "ERROR\n");

  carmen_param_subscribe_double("robot", "frontlaser_offset",
			      &tmp_frontlaser_offset, NULL);
  carmen_param_subscribe_double("robot", "front_laser_max",
			      &tmp_max_range, NULL);
}

static gint updateIPC(gpointer *data __attribute__ ((unused))) {

  sleep_ipc(0.01);
  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);

  return 1;
}

int main(int argc, char *argv[]) {

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  gtk_init(&argc, &argv);

  params_init(argc, argv);
  gui_init();
  scan_init();

  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);

  gtk_main();

  return 0;
}
