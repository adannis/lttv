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
#ifndef _TIME_ENTRY_
#define _TIME_ENTRY_


#include <glib.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtklabel.h>

G_BEGIN_DECLS

#define TIMEENTRY_TYPE            (timeentry_get_type ())
#define TIMEENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TIMEENTRY_TYPE, Timeentry))
#define TIMEENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TIMEENTRY_TYPE, TimeentryClass))
#define IS_TIMEENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TIMEENTRY_TYPE))
#define IS_TIMEENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TIMEENTRY_TYPE))

typedef struct _Timeentry       Timeentry;
typedef struct _TimeentryClass  TimeentryClass;

struct _Timeentry {
	GtkHBox hbox;

	GtkWidget *main_label;
	GtkWidget *main_label_box;
	GtkWidget *main_label_context_menu;
	GtkWidget *seconds_spinner;
	GtkWidget *nanoseconds_spinner;
	GtkWidget *s_label;
	GtkWidget *ns_label;

	int seconds_changed_handler_id;
	int nanoseconds_changed_handler_id;

	unsigned long min_seconds;
	unsigned long min_nanoseconds;
	unsigned long max_seconds;
	unsigned long max_nanoseconds;
};

struct _TimeentryClass {
	GtkHBoxClass parent_class;

	void (*timeentry) (Timeentry *timeentry);
};

GType timeentry_get_type(void);
GtkWidget *timeentry_new(const gchar *label);

void timeentry_set_main_label(Timeentry *timeentry,
			const gchar *str);
void timeentry_set_minmax_time(Timeentry *timeentry,
			unsigned long min_seconds,
			unsigned long min_nanoseconds,
			unsigned long max_seconds,
			unsigned long max_nanoseconds);

void timeentry_set_time(Timeentry *timeentry,
			unsigned long seconds,
			unsigned long nanoseconds);

void timeentry_get_time(Timeentry *timeentry,
			unsigned long *seconds,
			unsigned long *nanoseconds);
G_END_DECLS

#endif /* _TIME_ENTRY_ */
