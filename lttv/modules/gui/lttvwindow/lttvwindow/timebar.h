/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2010 Yannick Brosseau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */
#ifndef _TIMEBAR_
#define _TIMEBAR_

#include <glib.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtkwidget.h>
#include <lttv/time.h>

G_BEGIN_DECLS

#define TIMEBAR_TYPE            (timebar_get_type ())
#define TIMEBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TIMEBAR_TYPE, Timebar))
#define TIMEBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TIMEBAR_TYPE, TimebarClass))
#define IS_TIMEBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TIMEBAR_TYPE))
#define IS_TIMEBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TIMEBAR_TYPE))

typedef struct _Timebar Timebar;
typedef struct _TimebarClass  TimebarClass;

struct _Timebar {
	GtkHBox parent_hbox;

	GtkWidget *title_label;
	GtkWidget *title_eventbox;
	GtkWidget *start_timeentry;
	GtkWidget *end_timeentry;
	GtkWidget *interval_timeentry;
	GtkWidget *current_timeentry;

	int interval_handler_id;

	/* Time management */
	LttTime min_time;
	LttTime max_time;
};

struct _TimebarClass {
	GtkHBoxClass parent_class;

	void (*timebar) (Timebar *timebar);
};


GType timebar_get_type(void);
GtkWidget *timebar_new(void);

void timebar_set_current_time(Timebar *timeebar, const LttTime *time);
void timebar_set_start_time(Timebar *timebar, const LttTime *time);
void timebar_set_end_time(Timebar *timebar, const LttTime *time);
void timebar_set_minmax_time(Timebar *timebar, 
			const LttTime *min_time,
			const LttTime *max_time);

LttTime timebar_get_current_time(Timebar *timeebar);
LttTime timebar_get_start_time(Timebar *timebar);
LttTime timebar_get_end_time(Timebar *timebar);

G_END_DECLS

#endif /* _TIMEBAR_ */
