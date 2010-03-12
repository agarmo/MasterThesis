/*********************************************************
 *
 * This source code is part of the Carnegie Mellon Robot
 * Navigation Toolkit (CARMEN)
 *
 * CARMEN Copyright (c) 2002 Michael Montemerlo, Nicholas
 * Roy, and Sebastian Thrun
 *
 * CARMEN is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public 
 * License as published by the Free Software Foundation; 
 * either version 2 of the License, or (at your option)
 * any later version.
 *
 * CARMEN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied 
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General 
 * Public License along with CARMEN; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, 
 * Suite 330, Boston, MA  02111-1307 USA
 *
 ********************************************************/

/********************************************************
 *
 * The CARMEN Canon module was written by 
 * Mike Montemerlo (mmde+@cs.cmu.edu)
 * Chunks of code were taken from the Canon library that
 * is part of Gphoto2.  http://www.gphoto.com
 *
 ********************************************************/

#include <carmen/carmen_graphics.h>
#include "canon_interface.h"
#include "jpegread.h"

GtkWidget *drawing_area;
GdkGC *gc = NULL;

unsigned char *image = NULL;
int image_rows, image_cols;

void redraw(void)
{
  if(image != NULL)
    gdk_draw_rgb_image(drawing_area->window, gc, 0, 0, 320, 240, 
		       GDK_RGB_DITHER_NONE, image, image_cols * 3);
}

void canon_preview_handler(carmen_canon_preview_message *preview)
{
  static int image_count = 0;
  static double first_time = 0;

  if(image_count == 0)
    first_time = carmen_get_time_ms();
  image_count++;
  if(image_count % 30 == 0)
    fprintf(stderr, "Running at %.2f fps.\n", image_count / 
	    (carmen_get_time_ms() - first_time));
  if(image != NULL)
    free(image);
  read_jpeg_from_memory(preview->preview, preview->preview_length, 
			&image, &image_cols, &image_rows);
  redraw();
}

static gint expose_event(GtkWidget *widget __attribute__ ((unused)),
                         GdkEventExpose *event __attribute__ ((unused)))
{
  redraw();
  return FALSE;
}

static gint updateIPC(gpointer *data __attribute__ ((unused))) 
{
  sleep_ipc(0.01);
  carmen_graphics_update_ipc_callbacks((GdkInputFunction)updateIPC);
  return 1;
}

GtkWidget *create_drawing_window(char *name, int x, int y,
                                 int width, int height, GtkSignalFunc expose)
{
  GtkWidget *main_window, *drawing_area;
  
  /* Create the top-level window */
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(main_window), name);
  gtk_signal_connect(GTK_OBJECT(main_window), "delete_event",
                     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_widget_set_uposition(main_window, x, y);
  /* Create the drawing area */
  drawing_area = gtk_drawing_area_new();
  gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event", expose, NULL);
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), width, height);
  gtk_container_add(GTK_CONTAINER(main_window), drawing_area);
  gtk_widget_show(drawing_area);
  /* Show the top-level window */
  gtk_widget_show(main_window);
  return drawing_area;
}

void initialize_graphics(int *argc, char ***argv)
{
  gtk_init(argc, argv);
  gdk_imlib_init();
  gdk_rgb_init();
  carmen_graphics_update_ipc_callbacks((GdkInputFunction)updateIPC);
  carmen_graphics_initialize_screenshot();
  carmen_graphics_setup_colors();
  drawing_area = create_drawing_window("previewer", 50, 50, 320, 240,
                                       (GtkSignalFunc)expose_event);
  gc = gdk_gc_new(drawing_area->window);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event", 
                     (GtkSignalFunc)expose_event, NULL);
}

void shutdown_module(int x)
{
  if(x == SIGINT)
    gtk_main_quit();
}

int main(int argc, char **argv)
{
  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);
  carmen_canon_subscribe_preview_message(NULL, (carmen_handler_t)
					 canon_preview_handler,
					 CARMEN_SUBSCRIBE_LATEST);
  carmen_canon_start_preview_command();
  signal(SIGINT, shutdown_module);
  initialize_graphics(&argc, &argv);
  gtk_main();
  carmen_canon_stop_preview_command();
  close_ipc();
  return 0;
}
