
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <lttv/processTrace.h>
#include <lttv/gtkTraceSet.h>
#include <lttv/hook.h>

#include "Drawing.h"
#include "CFV.h"
#include "CFV-private.h"
#include "Event_Hooks.h"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

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
  { 0, 0xffff, 0x0000, 0x0000 },  // RED
  { 0, 0x0000, 0xffff, 0x0000 },  // GREEN
  { 0, 0x0000, 0x0000, 0xffff },  // BLUE
  { 0, 0xffff, 0xffff, 0xffff },  // WHITE
  { 0, 0x0000, 0x0000, 0x0000 } // BLACK
};


/* Function responsible for updating the exposed area.
 * It must call processTrace() to ask for this update.
 * Note : this function cannot clear the background, because it may
 * erase drawing already present (SAFETY).
 */
void drawing_data_request(Drawing_t *Drawing,
      GdkPixmap **pixmap,
      gint x, gint y,
      gint width,
      gint height)
{
  if(width < 0) return ;
  if(height < 0) return ;
  ControlFlowData *control_flow_data =
      (ControlFlowData*)g_object_get_data(
                G_OBJECT(
                    Drawing->drawing_area),
                "control_flow_data");

  LttTime start, end;
  LttTime window_end = ltt_time_add(control_flow_data->time_window.time_width,
                        control_flow_data->time_window.start_time);

  g_critical("req : window_end : %u, %u", window_end.tv_sec, 
                                      window_end.tv_nsec);

  g_critical("req : time width : %u, %u", control_flow_data->time_window.time_width.tv_sec, 
                                control_flow_data->time_window.time_width.tv_nsec);
  
  g_critical("x is : %i, x+width is : %i", x, x+width);

  convert_pixels_to_time(Drawing->drawing_area->allocation.width, x,
        &control_flow_data->time_window.start_time,
        &window_end,
        &start);

  convert_pixels_to_time(Drawing->drawing_area->allocation.width, x + width,
        &control_flow_data->time_window.start_time,
        &window_end,
        &end);
  
  LttvTracesetContext * tsc =
        get_traceset_context(control_flow_data->mw);
  
    //send_test_process(
  //guicontrolflow_get_process_list(Drawing->control_flow_data),
  //Drawing);
  //send_test_drawing(
  //guicontrolflow_get_process_list(Drawing->control_flow_data),
  //Drawing, *pixmap, x, y, width, height);
  
  // Let's call processTrace() !!
  EventRequest event_request; // Variable freed at the end of the function.
  event_request.control_flow_data = control_flow_data;
  event_request.time_begin = start;
  event_request.time_end = end;
  event_request.x_begin = x;
  event_request.x_end = x+width;

  g_critical("req : start : %u, %u", event_request.time_begin.tv_sec, 
                                      event_request.time_begin.tv_nsec);

  g_critical("req : end : %u, %u", event_request.time_end.tv_sec, 
                                      event_request.time_end.tv_nsec);
  
  LttvHooks *event = lttv_hooks_new();
  LttvHooks *after_event = lttv_hooks_new();
  LttvHooks *after_traceset = lttv_hooks_new();
  lttv_hooks_add(after_traceset, after_data_request, &event_request);
  lttv_hooks_add(event, draw_event_hook, &event_request);
  //Modified by xiangxiu: state update hooks are added by the main window
  //state_add_event_hooks_api(control_flow_data->mw);
  lttv_hooks_add(after_event, draw_after_hook, &event_request);

  lttv_process_traceset_seek_time(tsc, start);
  // FIXME : would like to place the after_traceset hook after the traceset,
  // but the traceset context state is not valid anymore.
  lttv_traceset_context_add_hooks(tsc,
  //    NULL, after_traceset, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, after_traceset, NULL, event, after_event);
  lttv_process_traceset(tsc, end, G_MAXULONG);
  //after_data_request((void*)&event_request,(void*)tsc);
  lttv_traceset_context_remove_hooks(tsc,
      //NULL, after_traceset, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, after_traceset, NULL, event, after_event);
  //Modified by xiangxiu: state update hooks are removed by the main window
  //state_remove_event_hooks_api(control_flow_data->mw);

  lttv_hooks_destroy(after_traceset);
  lttv_hooks_destroy(event);
  lttv_hooks_destroy(after_event);

  
}
          
/* Callbacks */


/* Create a new backing pixmap of the appropriate size */
/* As the scaling will always change, it's of no use to copy old
 * pixmap.
 */
static gboolean
configure_event( GtkWidget *widget, GdkEventConfigure *event, 
    gpointer user_data)
{
  Drawing_t *Drawing = (Drawing_t*)user_data;


  /* First, get the new time interval of the main window */
  /* we assume (see documentation) that the main window
   * has updated the time interval before this configure gets
   * executed.
   */
  get_time_window(Drawing->control_flow_data->mw,
        &Drawing->control_flow_data->time_window);
  
  /* New pixmap, size of the configure event */
  //GdkPixmap *pixmap = gdk_pixmap_new(widget->window,
  //      widget->allocation.width + SAFETY,
  //      widget->allocation.height + SAFETY,
  //      -1);
  
  g_critical("drawing configure event");
  g_critical("New draw size : %i by %i",widget->allocation.width, widget->allocation.height);
  
    
  if (Drawing->pixmap)
    gdk_pixmap_unref(Drawing->pixmap);
  
  /* If no old pixmap present */
  //if(Drawing->pixmap == NULL)
  {
    Drawing->pixmap = gdk_pixmap_new(
    widget->window,
    widget->allocation.width + SAFETY,
    widget->allocation.height + SAFETY,
    //ProcessList_get_height
    // (GuiControlFlow_get_process_list(Drawing->control_flow_data)),
    -1);
    Drawing->width = widget->allocation.width;
    Drawing->height = widget->allocation.height;
    

    // Clear the image
    gdk_draw_rectangle (Drawing->pixmap,
          widget->style->white_gc,
          TRUE,
          0, 0,
          widget->allocation.width+SAFETY,
          widget->allocation.height+SAFETY);

    //g_info("init data request");


    /* Initial data request */
    // Do not need to ask for data of 1 pixel : not synchronized with
    // main window time at this moment.
    drawing_data_request(Drawing, &Drawing->pixmap, 0, 0,
        widget->allocation.width,
        widget->allocation.height);
                          
    Drawing->width = widget->allocation.width;
    Drawing->height = widget->allocation.height;

    return TRUE;



  }
#ifdef NOTUSE
//  /* Draw empty background */ 
//  gdk_draw_rectangle (pixmap,
//          widget->style->black_gc,
//          TRUE,
//          0, 0,
//          widget->allocation.width,
//          widget->allocation.height);
  
  /* Copy old data to new pixmap */
  gdk_draw_drawable (pixmap,
    widget->style->white_gc,
    Drawing->pixmap,
    0, 0,
    0, 0,
    -1, -1);
    
  if (Drawing->pixmap)
    gdk_pixmap_unref(Drawing->pixmap);

  Drawing->pixmap = pixmap;
    
  // Clear the bottom part of the image (SAFETY)
  gdk_draw_rectangle (pixmap,
          widget->style->white_gc,
          TRUE,
          0, Drawing->height+SAFETY,
          Drawing->width+SAFETY,  // do not overlap
          (widget->allocation.height) - Drawing->height);

  // Clear the right part of the image (SAFETY)
  gdk_draw_rectangle (pixmap,
          widget->style->white_gc,
          TRUE,
          Drawing->width+SAFETY, 0,
          (widget->allocation.width) - Drawing->width,  // do not overlap
          Drawing->height+SAFETY);

  /* Clear the backgound for data request, but not SAFETY */
  gdk_draw_rectangle (pixmap,
          Drawing->drawing_area->style->white_gc,
          TRUE,
          Drawing->width + SAFETY, 0,
          widget->allocation.width - Drawing->width,  // do not overlap
          widget->allocation.height+SAFETY);

  /* Request data for missing space */
  g_info("missing data request");
  drawing_data_request(Drawing, &pixmap, Drawing->width, 0,
      widget->allocation.width - Drawing->width,
      widget->allocation.height);
                          
  Drawing->width = widget->allocation.width;
  Drawing->height = widget->allocation.height;

  return TRUE;
#endif //NOTUSE
}


/* Redraw the screen from the backing pixmap */
static gboolean
expose_event( GtkWidget *widget, GdkEventExpose *event, gpointer user_data )
{
  Drawing_t *Drawing = (Drawing_t*)user_data;
  ControlFlowData *control_flow_data =
      (ControlFlowData*)g_object_get_data(
                G_OBJECT(widget),
                "control_flow_data");

  g_critical("drawing expose event");
  
  guint x=0;
  LttTime* current_time = 
      guicontrolflow_get_current_time(control_flow_data);

  LttTime window_end = ltt_time_add(control_flow_data->time_window.time_width,
                      control_flow_data->time_window.start_time);

  convert_time_to_pixels(
        control_flow_data->time_window.start_time,
        window_end,
        *current_time,
        widget->allocation.width,
        &x);
  
  gdk_draw_pixmap(widget->window,
      widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
      Drawing->pixmap,
      event->area.x, event->area.y,
      event->area.x, event->area.y,
      event->area.width, event->area.height);

  if(x >= event->area.x && x <= event->area.x+event->area.width)
  {
    GdkGC *gc = gdk_gc_new(control_flow_data->Drawing->pixmap);
    gdk_gc_copy(gc, widget->style->black_gc);
    
    drawing_draw_line(NULL, widget->window,
                  x, event->area.y,
                  x, event->area.y+event->area.height,
                  gc);
    gdk_gc_unref(gc);
  }
  return FALSE;
}

/* mouse click */
static gboolean
button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
  ControlFlowData *control_flow_data =
      (ControlFlowData*)g_object_get_data(
                G_OBJECT(widget),
                "control_flow_data");
  Drawing_t *Drawing = control_flow_data->Drawing;


  g_critical("click");
  if(event->button == 1)
  {
    LttTime time;

    LttTime window_end = ltt_time_add(control_flow_data->time_window.time_width,
                        control_flow_data->time_window.start_time);


    /* left mouse button click */
    g_critical("x click is : %f", event->x);

    convert_pixels_to_time(widget->allocation.width, (guint)event->x,
        &control_flow_data->time_window.start_time,
        &window_end,
        &time);

    set_current_time(control_flow_data->mw, &time);

  }
  
  return FALSE;
}




Drawing_t *drawing_construct(ControlFlowData *control_flow_data)
{
  Drawing_t *Drawing = g_new(Drawing_t, 1);
    
  Drawing->drawing_area = gtk_drawing_area_new ();
  Drawing->control_flow_data = control_flow_data;

  Drawing->pango_layout =
    gtk_widget_create_pango_layout(Drawing->drawing_area, NULL);
  
  //gtk_widget_set_size_request(Drawing->drawing_area->window, 50, 50);
  g_object_set_data_full(
      G_OBJECT(Drawing->drawing_area),
      "Link_Drawing_Data",
      Drawing,
      (GDestroyNotify)drawing_destroy);

  //gtk_widget_modify_bg( Drawing->drawing_area,
  //      GTK_STATE_NORMAL,
  //      &CF_Colors[BLACK]);
  
  //gdk_window_get_geometry(Drawing->drawing_area->window,
  //    NULL, NULL,
  //    &(Drawing->width),
  //    &(Drawing->height),
  //    -1);
  
  //Drawing->pixmap = gdk_pixmap_new(
  //    Drawing->drawing_area->window,
  //    Drawing->width,
  //    Drawing->height,
  //    Drawing->depth);
  
  Drawing->pixmap = NULL;

//  Drawing->pixmap = gdk_pixmap_new(Drawing->drawing_area->window,
//        Drawing->drawing_area->allocation.width,
//        Drawing->drawing_area->allocation.height,
//        -1);

  gtk_widget_add_events(Drawing->drawing_area, GDK_BUTTON_PRESS_MASK);
  
  g_signal_connect (G_OBJECT(Drawing->drawing_area),
        "configure_event",
        G_CALLBACK (configure_event),
        (gpointer)Drawing);
  
  g_signal_connect (G_OBJECT(Drawing->drawing_area),
        "expose_event",
        G_CALLBACK (expose_event),
        (gpointer)Drawing);

  g_signal_connect (G_OBJECT(Drawing->drawing_area),
        "button-press-event",
        G_CALLBACK (button_press_event),
        (gpointer)Drawing);

  
  return Drawing;
}

void drawing_destroy(Drawing_t *Drawing)
{

  // Do not unref here, Drawing_t destroyed by it's widget.
  //g_object_unref( G_OBJECT(Drawing->drawing_area));
    
  g_free(Drawing->pango_layout);
  g_free(Drawing);
}

GtkWidget *drawing_get_widget(Drawing_t *Drawing)
{
  return Drawing->drawing_area;
}

/* convert_pixels_to_time
 *
 * Convert from window pixel and time interval to an absolute time.
 */
void convert_pixels_to_time(
    gint width,
    guint x,
    LttTime *window_time_begin,
    LttTime *window_time_end,
    LttTime *time)
{
  LttTime window_time_interval;
  
  window_time_interval = ltt_time_sub(*window_time_end, 
            *window_time_begin);
  *time = ltt_time_mul(window_time_interval, (x/(float)width));
  *time = ltt_time_add(*window_time_begin, *time);
}



void convert_time_to_pixels(
    LttTime window_time_begin,
    LttTime window_time_end,
    LttTime time,
    int width,
    guint *x)
{
  LttTime window_time_interval;
  float interval_float, time_float;
  
  window_time_interval = ltt_time_sub(window_time_end,window_time_begin);
  
  time = ltt_time_sub(time, window_time_begin);
  
  interval_float = ltt_time_to_double(window_time_interval);
  time_float = ltt_time_to_double(time);

  *x = (guint)(time_float/interval_float * width);
  
}

void drawing_refresh (  Drawing_t *Drawing,
      guint x, guint y,
      guint width, guint height)
{
  g_info("Drawing.c : drawing_refresh %u, %u, %u, %u", x, y, width, height);
  GdkRectangle update_rect;

  gdk_draw_drawable(
    Drawing->drawing_area->window,
    Drawing->drawing_area->
     style->fg_gc[GTK_WIDGET_STATE (Drawing->drawing_area)],
    GDK_DRAWABLE(Drawing->pixmap),
    x, y,
    x, y,
    width, height);

  update_rect.x = 0 ;
  update_rect.y = 0 ;
  update_rect.width = Drawing->width;
  update_rect.height = Drawing->height ;
  gtk_widget_draw( Drawing->drawing_area, &update_rect);

}


void drawing_draw_line( Drawing_t *Drawing,
      GdkPixmap *pixmap,
      guint x1, guint y1,
      guint x2, guint y2,
      GdkGC *GC)
{
  gdk_draw_line (pixmap,
      GC,
      x1, y1, x2, y2);
}




void drawing_resize(Drawing_t *Drawing, guint h, guint w)
{
  Drawing->height = h ;
  Drawing->width = w ;

  gtk_widget_set_size_request ( Drawing->drawing_area,
          Drawing->width,
          Drawing->height);
  
  
}


/* Insert a square corresponding to a new process in the list */
/* Applies to whole Drawing->width */
void drawing_insert_square(Drawing_t *Drawing,
        guint y,
        guint height)
{
  //GdkRectangle update_rect;

  /* Allocate a new pixmap with new height */
  GdkPixmap *pixmap = gdk_pixmap_new(Drawing->drawing_area->window,
        Drawing->width + SAFETY,
        Drawing->height + height + SAFETY,
        -1);
  
  /* Copy the high region */
  gdk_draw_drawable (pixmap,
    Drawing->drawing_area->style->black_gc,
    Drawing->pixmap,
    0, 0,
    0, 0,
    Drawing->width + SAFETY, y);




  /* add an empty square */
  gdk_draw_rectangle (pixmap,
    Drawing->drawing_area->style->white_gc,
    TRUE,
    0, y,
    Drawing->width + SAFETY,  // do not overlap
    height);



  /* copy the bottom of the region */
  gdk_draw_drawable (pixmap,
    Drawing->drawing_area->style->black_gc,
    Drawing->pixmap,
    0, y,
    0, y + height,
    Drawing->width+SAFETY, Drawing->height - y + SAFETY);




  if (Drawing->pixmap)
    gdk_pixmap_unref(Drawing->pixmap);

  Drawing->pixmap = pixmap;
  
  Drawing->height+=height;

  /* Rectangle to update, from new Drawing dimensions */
  //update_rect.x = 0 ;
  //update_rect.y = y ;
  //update_rect.width = Drawing->width;
  //update_rect.height = Drawing->height - y ;
  //gtk_widget_draw( Drawing->drawing_area, &update_rect);
}


/* Remove a square corresponding to a removed process in the list */
void drawing_remove_square(Drawing_t *Drawing,
        guint y,
        guint height)
{
  //GdkRectangle update_rect;
  
  /* Allocate a new pixmap with new height */
  GdkPixmap *pixmap = gdk_pixmap_new(
      Drawing->drawing_area->window,
      Drawing->width + SAFETY,
      Drawing->height - height + SAFETY,
      -1);
  
  /* Copy the high region */
  gdk_draw_drawable (pixmap,
    Drawing->drawing_area->style->black_gc,
    Drawing->pixmap,
    0, 0,
    0, 0,
    Drawing->width + SAFETY, y);



  /* Copy up the bottom of the region */
  gdk_draw_drawable (pixmap,
    Drawing->drawing_area->style->black_gc,
    Drawing->pixmap,
    0, y + height,
    0, y,
    Drawing->width, Drawing->height - y - height + SAFETY);


  if (Drawing->pixmap)
    gdk_pixmap_unref(Drawing->pixmap);

  Drawing->pixmap = pixmap;
  
  Drawing->height-=height;
  
  /* Rectangle to update, from new Drawing dimensions */
  //update_rect.x = 0 ;
  //update_rect.y = y ;
  //update_rect.width = Drawing->width;
  //update_rect.height = Drawing->height - y ;
  //gtk_widget_draw( Drawing->drawing_area, &update_rect);
}


