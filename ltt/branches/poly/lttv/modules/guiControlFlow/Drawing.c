
#include "Drawing.h"
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <lttv/processTrace.h>

/*****************************************************************************
 *                              Drawing functions                            *
 *****************************************************************************/

//FIXME Colors will need to be dynamic. Graphic context part not done so far.
typedef enum 
{
	RED,
	GREEN,
	BLUE,
	WHITE,
	BLACK

} ControlFlowColors;

/* Vector of unallocated colors */
static GdkColor CF_Colors [] = 
{
	{ 0, 0xffff, 0x0000, 0x0000 },	// RED
	{ 0, 0x0000, 0xffff, 0x0000 },	// GREEN
	{ 0, 0x0000, 0x0000, 0xffff },	// BLUE
	{ 0, 0xffff, 0xffff, 0xffff },	// WHITE
	{ 0, 0x0000, 0x0000, 0x0000 }	// BLACK
};


struct _Drawing_t {
	GtkWidget	*Drawing_Area_V;
	GdkPixmap	*Pixmap;

	gint 		height, width, depth;

};

/* Function responsible for updating the exposed area.
 * It must call processTrace() to ask for this update.
 */
void Drawing_Data_Request(Drawing_t *Drawing,
			GdkPixmap *Pixmap,
			gint x, gint y,
		   	gint width,
			gint height)
{
  if(width < 0) return ;
  if(height < 0) return ;

  gdk_draw_rectangle (Pixmap,
		      Drawing->Drawing_Area_V->style->white_gc,
		      TRUE,
		      x, y,
		      width,	// do not overlap
		      height);

  Drawing_draw_line(Drawing, Pixmap, 10, 10, 50, 10,
			Drawing->Drawing_Area_V->style->black_gc);

  
}
		      
/* Callbacks */


/* Create a new backing pixmap of the appropriate size */
static gboolean
configure_event( GtkWidget *widget, GdkEventConfigure *event, 
		gpointer user_data)
{
  Drawing_t *Drawing = (Drawing_t*)user_data;

  GdkPixmap *Pixmap = gdk_pixmap_new(widget->window,
			  widget->allocation.width,
			  widget->allocation.height,
			  -1);
	
  if(Drawing->Pixmap == NULL)
  {
  	Drawing->Pixmap = gdk_pixmap_new(widget->window,
			  	widget->allocation.width,
			  	widget->allocation.height,
			  	-1);
	Drawing->width = widget->allocation.width;
	Drawing->height = widget->allocation.height;

	/* Initial data request */
 	Drawing_Data_Request(Drawing, Drawing->Pixmap, 0, 0,
		   	widget->allocation.width,
			widget->allocation.height);

  }
//  /* Draw empty background */ 
//  gdk_draw_rectangle (Pixmap,
//		      widget->style->black_gc,
//		      TRUE,
//		      0, 0,
//		      widget->allocation.width,
//		      widget->allocation.height);
  
  /* Copy old data to new pixmap */
  gdk_draw_drawable (Pixmap,
		  widget->style->white_gc,
		  Drawing->Pixmap,
		  0, 0,
		  0, 0,
		  -1, -1);

   /* Request data for missing space */
   Drawing_Data_Request(Drawing, Pixmap, Drawing->width, 0,
		   	widget->allocation.width - Drawing->width,
			widget->allocation.height);
   Drawing_Data_Request(Drawing, Pixmap, 0, Drawing->height,
		   Drawing->width,
		   widget->allocation.height - Drawing->height);
			                      
//   gdk_draw_rectangle (Pixmap,
//		      widget->style->white_gc,
//		      TRUE,
//		      Drawing->width, 0,
//		      widget->allocation.width -
//		      			Drawing->width,
//		      widget->allocation.height);

//    gdk_draw_rectangle (Pixmap,
//		      widget->style->white_gc,
//		      TRUE,
//		      0, Drawing->height,
//		      Drawing->width,	// do not overlap
//		      widget->allocation.height -
//		      			Drawing->height);
 

  
  g_critical("drawing configure event");

  
  if (Drawing->Pixmap)
    gdk_pixmap_unref(Drawing->Pixmap);

  Drawing->Pixmap = Pixmap;
  Drawing->width = widget->allocation.width;
  Drawing->height = widget->allocation.height;

  return TRUE;
}


/* Redraw the screen from the backing pixmap */
static gboolean
expose_event( GtkWidget *widget, GdkEventExpose *event, gpointer user_data )
{
  Drawing_t *Drawing = (Drawing_t*)user_data;
  g_critical("drawing expose event");

  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  Drawing->Pixmap,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

Drawing_t *Drawing_construct(void)
{
	Drawing_t *Drawing = g_new(Drawing_t, 1);
		
	Drawing->Drawing_Area_V = gtk_drawing_area_new ();
	
	//gtk_widget_set_size_request(Drawing->Drawing_Area_V->window, 50, 50);
	g_object_set_data_full(
			G_OBJECT(Drawing->Drawing_Area_V),
			"Link_Drawing_Data",
			Drawing,
			(GDestroyNotify)Drawing_destroy);

	//gtk_widget_modify_bg(	Drawing->Drawing_Area_V,
	//			GTK_STATE_NORMAL,
	//			&CF_Colors[BLACK]);
	
	//gdk_window_get_geometry(Drawing->Drawing_Area_V->window,
	//		NULL, NULL,
	//		&(Drawing->width),
	//		&(Drawing->height),
	//		-1);
	
	//Drawing->Pixmap = gdk_pixmap_new(
	//		Drawing->Drawing_Area_V->window,
	//		Drawing->width,
	//		Drawing->height,
	//		Drawing->depth);
	
	Drawing->Pixmap = NULL;

// 	Drawing->Pixmap = gdk_pixmap_new(Drawing->Drawing_Area_V->window,
//			  Drawing->Drawing_Area_V->allocation.width,
//			  Drawing->Drawing_Area_V->allocation.height,
//			  -1);


	g_signal_connect (G_OBJECT(Drawing->Drawing_Area_V),
				"configure_event",
				G_CALLBACK (configure_event),
				(gpointer)Drawing);
	
	g_signal_connect (G_OBJECT(Drawing->Drawing_Area_V),
				"expose_event",
				G_CALLBACK (expose_event),
				(gpointer)Drawing);

	return Drawing;
}

void Drawing_destroy(Drawing_t *Drawing)
{

	// Do not unref here, Drawing_t destroyed by it's widget.
	//g_object_unref( G_OBJECT(Drawing->Drawing_Area_V));
		
	g_free(Drawing);
}

GtkWidget *Drawing_getWidget(Drawing_t *Drawing)
{
	return Drawing->Drawing_Area_V;
}

/* get_time_from_pixels
 *
 * Get the time interval from window time and pixels, and pixels requested. This
 * function uses TimeMul, which should only be used if the float value is lower
 * that 4, and here it's always lower than 1, so it's ok.
 */
void convert_pixels_to_time(
		Drawing_t *Drawing,
		guint x,
		LttTime *window_time_begin,
		LttTime *window_time_end,
		LttTime *time)
{
	LttTime window_time_interval;
	
	TimeSub(window_time_interval, *window_time_end, *window_time_begin);

	
	TimeMul(*time, window_time_interval,
			(x/(float)Drawing->width));
	TimeAdd(*time, *window_time_begin, *time);
	
}



void convert_time_to_pixels(
		LttTime window_time_begin,
		LttTime window_time_end,
		LttTime time,
		Drawing_t *Drawing,
		guint *x)
{
	LttTime window_time_interval;
	float interval_float, time_float;
	
	TimeSub(window_time_interval, window_time_end, window_time_begin);
	
	TimeSub(time, time, window_time_begin);
	
	interval_float = (window_time_interval.tv_sec * NANSECOND_CONST)
			+ window_time_interval.tv_nsec;
	time_float = (time.tv_sec * NANSECOND_CONST)
			+ time.tv_nsec;

	*x = (guint)(time_float/interval_float * Drawing->width);
	
}

void Drawing_Refresh (	Drawing_t *Drawing,
			guint x, guint y,
			guint width, guint height)
{
	gdk_draw_drawable(
		Drawing->Drawing_Area_V->window,
		Drawing->Drawing_Area_V->
		 style->fg_gc[GTK_WIDGET_STATE (Drawing->Drawing_Area_V)],
		GDK_DRAWABLE(Drawing->Pixmap),
		x, y,
		x, y,
		width, height);
}


void Drawing_draw_line(	Drawing_t *Drawing,
			GdkPixmap *Pixmap,
			guint x1, guint y1,
			guint x2, guint y2,
			GdkGC *GC)
{
	gdk_draw_line (Pixmap,
			GC,
			x1, y1, x2, y2);
}




void Drawing_Resize(Drawing_t *Drawing, guint h, guint w)
{
	Drawing->height = h ;
	Drawing->width = w ;

	gtk_widget_set_size_request (	Drawing->Drawing_Area_V,
					Drawing->width,
					Drawing->height);
	
	
}


