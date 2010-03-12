#include <qapplication.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <carmen/carmen.h>
#include <carmen/proccontrol_interface.h>

#ifdef __cplusplus
}
#endif

#include "proccontrol_gui.h"

QDisplay                             * qdisplay;
int                                    pid_update = FALSE;
int                                    out_update = FALSE;

process_table_type                     table;
carmen_proccontrol_pidtable_message     pidtable;  
carmen_proccontrol_output_message       output;

QDisplay::QDisplay( QWidget *parent, const char *name )
 : QWidget( parent, name )
{
  int i, j;

  setCaption( "PROCCONTROL GUI" );

  QVBoxLayout  *vbox = new QVBoxLayout( this );

  for (j=0; j<MAX_NUM_GROUPS; j++) {
    box[j] = new QHBoxLayout( vbox );
    QString s;
    s.sprintf( "Group (%d)", j );
    bgrp[j] = new QButtonGroup( 1,  QGroupBox::Vertical, s, this );
    box[j]->addWidget( bgrp[j] );
    {
      but[j][0] = new QPushButton( bgrp[j] );
      QString s;
      but[j][0]->setText( "All" );
      but[j][0]->setMinimumWidth( 60 );
      but[j][0]->setMaximumWidth( 60 );
      {
	QPopupMenu *menu = new QPopupMenu(but[j][0]);
	menu->insertItem("Start", this, SLOT( startClicked(int) ),
			 0, (NUM_STATES*(j*MAX_NUM_MODULES))+0 );
	menu->insertItem("Stop", this, SLOT( stopClicked(int) ),
			 0, (NUM_STATES*(j*MAX_NUM_MODULES))+1 );
	but[j][0]->setPopup(menu);
      }
    }
    for (i=1; i<MAX_NUM_MODULES; i++) { 
      but[j][i] = new QPushButton( bgrp[j] );
      QString s;
      s.sprintf( "Button\n(%d)", i );
      but[j][i]->setText( s );
      but[j][i]->setMaximumWidth( 100 );
      but[j][i]->setToggleButton( TRUE );
      {
	QPopupMenu *menu = new QPopupMenu(but[j][i]);
	menu->insertItem("Start Program", this, SLOT( startClicked(int) ),
			 0, (NUM_STATES*(j*MAX_NUM_MODULES+i))+0 );
	menu->insertItem("Stop Program", this, SLOT( stopClicked(int) ),
			 0, (NUM_STATES*(j*MAX_NUM_MODULES+i))+1 );
	menu->insertItem("Show Output", this, SLOT( showClicked(int) ),
			 0, (NUM_STATES*(j*MAX_NUM_MODULES+i))+2 );
	menu->insertItem("No Output", this, SLOT( noClicked(int) ),
			 0, (NUM_STATES*(j*MAX_NUM_MODULES+i))+3 );
	but[j][i]->setPopup(menu);
      }
      but[j][i]->hide();
    }
    bgrp[j]->hide();
  }

  output =  new QTextView( this );
  output->setMaxLogLines(500);
  QFont font( "Courier" );
  font.setPointSize( 8 );
  output->setFont(font);
  vbox->addWidget( output ); 
 
  resize( 600, 400 );
}

void
QDisplay::closeEvent( QCloseEvent *ev )
{
  ev = NULL;
  exit(0);
}

void
QDisplay::showLine( char * text )
{
  output->append( text );
}

void
QDisplay::startClicked( int n )
{
  int g = (n/NUM_STATES)/MAX_NUM_MODULES;
  int m = (n/NUM_STATES)%MAX_NUM_MODULES;
  if (m==0) {
    fprintf( stderr, "INFO: start group %s\n",
	     table.process[g][m].group_name );
    carmen_proccontrol_set_group_state( table.process[g][m].group_name, 1 );
  } else {
    fprintf( stderr, "INFO: start module %s\n",
	     table.process[g][m-1].module_name );
    carmen_proccontrol_set_module_state( table.process[g][m-1].module_name, 1 );
  }
}

void
QDisplay::stopClicked( int n )
{
  int g = (n/NUM_STATES)/MAX_NUM_MODULES;
  int m = (n/NUM_STATES)%MAX_NUM_MODULES;
  if (m==0) {
    fprintf( stderr, "INFO: stop group %s\n",
	     table.process[g][m].group_name );
    carmen_proccontrol_set_group_state( table.process[g][m].group_name, 0 );
  } else {
    fprintf( stderr, "INFO: stop module %s\n",
	     table.process[g][m-1].module_name );
    carmen_proccontrol_set_module_state( table.process[g][m-1].module_name, 0 );
  }
}

void
QDisplay::showClicked( int n )
{
  int g = (n/NUM_STATES)/MAX_NUM_MODULES;
  int m = (n/NUM_STATES)%MAX_NUM_MODULES;
  table.process[g][m-1].output = TRUE;
  but[g][m]->setOn(1);
  if (!table.output) {
    carmen_output(TRUE);
    table.output = TRUE;
  }
}

void
QDisplay::noClicked( int n )
{
  int i, j;
  int g = (n/NUM_STATES)/MAX_NUM_MODULES;
  int m = (n/NUM_STATES)%MAX_NUM_MODULES;
  table.process[g][m-1].output = FALSE;
  but[g][m]->setOn(0);
  if (table.output) {
    table.output = 0;
    for (i=0; i<table.numgrps; i++) {
      for (j=0; j<table.procingrp[i]; j++) {
	table.output += table.process[i][j].output;
      }
    }
    if (table.output)
      table.output = TRUE;
    else
      carmen_output(FALSE);
  }
}

void
QDisplay::showStatus( int group, int module, int status )
{
  int i;
  if (group>=0 && group<MAX_NUM_GROUPS &&
      module>=0 && module<MAX_NUM_MODULES) {
    if (module==0) {
      for (i=0; i<MAX_NUM_MODULES; i++) {
	QPalette pal = but[group][i]->palette();
	if (status) {
	  pal.setColor(QColorGroup::Button, QColor(0, 255, 0));
	} else {
	  pal.setColor(QColorGroup::Button, QColor(255, 0, 0));
	}
	but[group][i]->setPalette(pal);
      }
    } else {
      QPalette pal = but[group][module]->palette();
      if (status) {
	pal.setColor(QColorGroup::Button, QColor(0, 255, 0));
      } else {
	pal.setColor(QColorGroup::Button, QColor(255, 0, 0));
      }
      but[group][module]->setPalette(pal);
    }
  }
}

void
QDisplay::setGroup( int group, char *group_name )
{
  if (group>=0 && group<MAX_NUM_GROUPS) {
    QString s( group_name );
    bgrp[group]->setTitle( s );
    bgrp[group]->show();
  }  
}

void
QDisplay::setModule( int group, int module, char * module_name, int pid )
{
  if (group>=0 && group<MAX_NUM_GROUPS &&
      module>=1 && module<MAX_NUM_MODULES) {
    QString s;
    s.sprintf( "%s\npid: %d", module_name, pid );
    but[group][module]->setText( s );
    but[group][module]->show();
  }
}

void
QDisplay::hideButton( int group, int module )
{
  if (module==0) {
    bgrp[group]->hide();
  } else {
    but[group][module]->setOn(0);
    but[group][module]->hide();
  }
}

/**********************************************************************
 *
 *
 *
 *
 *
 **********************************************************************/
int
output_pid( int pid )
{
  int i, j;
  for (i=0; i<table.numgrps; i++) {
    for (j=0; j<table.procingrp[i]; j++) {
      if (table.process[i][j].pid==pid) {
	return(table.process[i][j].output);
      }
    }
  }
  return(FALSE);
}

void
carmen_update_pidtable( carmen_proccontrol_pidtable_message *msg )
{
  static process_table_type  p;
  int                        i, j, g, m, newgrp;

  for (i=0; i<p.numgrps; i++) {
    p.procingrp[i] = 0;
  }
  p.numgrps = 0;
  
  for (i=0; i<msg->num_processes; i++) {
    newgrp = TRUE;
    for (j=0; j<p.numgrps; j++) {
      if (!strncmp(msg->process[i].group_name,p.process[j][0].group_name,
		   MAX_NAME_LENGTH)) {
	g = j;
	newgrp = FALSE;
	break;
      }
    }
    if (newgrp) {
      g = p.numgrps++;
    }
    m = p.procingrp[g];
    p.process[g][m].group = g;
    strncpy( p.process[g][m].group_name, msg->process[i].group_name,
	     MAX_NAME_LENGTH );
    p.process[g][m].module = m;
    strncpy( p.process[g][m].module_name, msg->process[i].module_name,
	     MAX_NAME_LENGTH );
    p.process[g][m].pid = msg->process[i].pid;
    p.process[g][m].active = msg->process[i].active;
    p.process[g][m].requested_state = msg->process[i].requested_state;
    p.procingrp[g]++;
  }
  for (i=0; i<table.numgrps; i++) {
    if (i>=p.numgrps || p.procingrp[i] != table.procingrp[i]) {
      for (j=0; j<table.procingrp[i]; j++) {
	qdisplay->hideButton( i, j );
	table.process[i][j].active = -1;
      }
    }
  }
  for (i=0; i<p.numgrps; i++) {
    if ( p.procingrp[i] != table.procingrp[i] ) {
      qdisplay->setGroup( i, p.process[i][0].group_name );
      for (j=0; j<p.procingrp[i]; j++) {
	table.process[i][j] = p.process[i][j];
	qdisplay->setModule( i, j+1,
			     table.process[i][j].module_name,
			     table.process[i][j].pid  );
	qdisplay->showStatus( i, j+1, table.process[i][j].active );
      }
      table.procingrp[i] = p.procingrp[i];
    } else {
      for (j=0; j<p.procingrp[i]; j++) {
	if ( strcmp(p.process[i][j].group_name,
		    table.process[i][j].group_name) ||
	     strcmp(p.process[i][j].module_name,
		    table.process[i][j].module_name) ||
	     p.process[i][j].pid != table.process[i][j].pid ||
	     p.process[i][j].active != table.process[i][j].active ||
	     p.process[i][j].requested_state !=
	     table.process[i][j].requested_state ) {
	  p.process[i][j].output = table.process[i][j].output;
	  table.process[i][j] = p.process[i][j];
	  qdisplay->setModule( i, j+1,
			       p.process[i][j].module_name,
			       p.process[i][j].pid  );
	  qdisplay->showStatus( i, j+1, p.process[i][j].active );
	}
      }
    }
  }
  table.numgrps = p.numgrps;
  
}

void
carmen_proccontrol_pidtable_handler( carmen_proccontrol_pidtable_message *msg __attribute__ ((unused)) )
{
  pid_update = TRUE;
}

void
carmen_output_handler( carmen_proccontrol_output_message *msg __attribute__ ((unused)) )
{
  out_update = TRUE;
}

void
carmen_output( int state )
{
  if (state) {
    fprintf( stderr, "INFO: subscribe output messages\n" );
    carmen_proccontrol_subscribe_output_message
      ( &output,
	(carmen_handler_t) carmen_output_handler,
	CARMEN_SUBSCRIBE_ALL );
  } else {
    fprintf( stderr, "INFO: unsubscribe output\n" );
    carmen_proccontrol_unsubscribe_output_message
      ( (carmen_handler_t) carmen_output_handler );
  }
}

void 
shutdown( int sig ) {
  exit(sig);
}

int
main( int argc, char** argv)
{
  QApplication         app( argc, argv );
  QDisplay             gui;


  qdisplay = &gui;
  
  carmen_ipc_initialize(argc, argv);

  carmen_proccontrol_subscribe_pidtable_message
    ( &pidtable,
      (carmen_handler_t ) carmen_proccontrol_pidtable_handler,
      CARMEN_SUBSCRIBE_ALL );

  signal(SIGINT,shutdown);
  gui.show();

  while (TRUE) {
    app.processEvents();
    if (pid_update) {
      carmen_update_pidtable( &pidtable );
      pid_update = FALSE;
    }
    if (out_update) {
      if (output_pid( output.pid ))
	qdisplay->showLine( output.output );
      out_update = FALSE;
    }
    carmen_ipc_sleep (0.02);
  }
}
