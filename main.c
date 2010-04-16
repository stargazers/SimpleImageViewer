 /*
 Simple Image Viewer with FIFO communication possibility.
 Copyright (C) 2010	Aleksi Räsänen <aleksi.rasanen@runosydan.net>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <gtk/gtk.h>

typedef struct
{
	// Main Window
	GtkWidget *win;

	// Image element
	GtkWidget *img;

	// EventBox
	GtkWidget *eb;

	// Application FIFO
	int fifo;

	// Filo to load
	char *file;

	// Are we fullscreened or not?
	gboolean fullscreen;

	guint width, height;
} APP;

void load_image( APP *app );

// Switch to fullscreen and back
void toggle_fullscreen( APP *app )
{
	GdkColor color;

	if( app->fullscreen )
	{
		gtk_window_unfullscreen( GTK_WINDOW( app->win ) );
		app->fullscreen = FALSE;
		gdk_color_parse( "#e7e5e4", &color );
	}
	else
	{
		gtk_window_fullscreen( GTK_WINDOW( app->win ) );
		app->fullscreen = TRUE;
		gdk_color_parse( "black", &color );
	}
	
	gtk_widget_modify_bg( app->eb, GTK_STATE_NORMAL, &color );
}

void load_image( APP *app )
{
	if( app->file == NULL )
		return;
	
	GdkPixbuf *pb;
	GError *err = NULL;

	pb = gdk_pixbuf_new_from_file_at_scale( app->file,
		app->width, app->height, TRUE, &err );
	
	if(! pb )
	{
		g_print( "Error while load pixbuf\n" );
		g_print( "Error: %s\n", err->message );
		return;
	}

	gtk_image_set_from_pixbuf( GTK_IMAGE( app->img ), pb );
	g_object_unref( pb );
}

// This will read if there is data coming from pipe,
// and if there is, read it and do things if so wanted.
gboolean read_from_pipe( APP *app )
{
	char buffer[2048];
	int readed = read( app->fifo, buffer, sizeof( buffer ) -2 );

	if( readed > 0 )
	{
		buffer[readed] = '\0';

		// Should we load image?
		if( strstr( buffer, "load" ) != 0 )
		{
			char filename[256], tmp[256];
			sscanf( buffer, "%s %s\n", &tmp, &filename );

			if( app->file != NULL )
				free( app->file );

			app->file = malloc( strlen( filename ) + 1 );
			sprintf( app->file, "%s", filename );
			load_image( app );
		}
		else if( strcmp( buffer, "fullscreen\n" ) == 0 )
		{
			toggle_fullscreen( app );
		}
	}

	return TRUE;
}

// Create user interface.
void create_ui( APP *app )
{
	app->win = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	app->eb = gtk_event_box_new();
	app->img = gtk_image_new();

	gtk_container_add( GTK_CONTAINER( app->eb ), app->img );
	gtk_container_add( GTK_CONTAINER( app->win ), app->eb );
	gtk_widget_show_all( app->win );
}

gboolean quit( GtkWidget *w, APP *app )
{
	unlink( "/tmp/iv_fifo" );
	gtk_main_quit();
}

gboolean key_press( GtkWidget *w, GdkEventKey *e, APP *app )
{
	switch( e->keyval )
	{
		case 'f':
			toggle_fullscreen( app );
			break;

		case 'q':
			quit( NULL, app );
			break;
	}
	
	return FALSE;
}

// This is called when window is resized
gboolean state_changed( GtkWidget *w, GdkEventConfigure *e, APP *app )
{
	app->width = e->width;
	app->height = e->height;
	load_image( app );
	return FALSE;
}

int main( int argc, char *argv[] )
{
	// Create communication channel, thank you.
	mkfifo( "/tmp/iv_fifo", 0700 );

	gtk_init( &argc, &argv );
	APP app;

	app.file = NULL;
	app.fifo = open( "/tmp/iv_fifo", O_RDONLY | O_NONBLOCK );
	app.fullscreen = FALSE;
	app.width = 300;
	app.height = 300;

	create_ui( &app );
	gtk_widget_set_size_request( GTK_WIDGET( app.win ), 300, 300 );

	g_signal_connect( G_OBJECT( app.win ), "key-press-event",
		G_CALLBACK( key_press ), &app );

	g_signal_connect( G_OBJECT( app.win ), "destroy",
		G_CALLBACK( quit ), &app );
	
	g_signal_connect( G_OBJECT( app.win ), "configure-event",
		G_CALLBACK( state_changed ), &app );

	// We call every 500ms function read_from_pipe so we can
	// get commands from our pipe.
	g_timeout_add( 500, (GSourceFunc)read_from_pipe, &app );

	// Load file if it is given as an argument
	if( argc == 2 )
	{
		app.file = malloc( strlen( argv[1] ) );
		sprintf( app.file, "%s", argv[1] );
		load_image( &app );
	}

	gtk_main();

	return 0;
}
