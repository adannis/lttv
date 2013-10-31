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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "timeentry.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <gtk/gtksignal.h>
#include <gtk/gtk.h>

enum {
	SIGNAL_TIME_CHANGED,
	LAST_SIGNAL
};

static void timeentry_class_init(TimeentryClass *klass);
static void timeentry_init(Timeentry *ttt);

static guint timeentry_signals[LAST_SIGNAL] = { 0 };
static unsigned int MAX_NANOSECONDS = 999999999;

static void on_spinner_value_changed (GtkSpinButton *spinbutton,
				gpointer user_data);

static gboolean on_label_click(GtkWidget *widget, 
			GdkEventButton *event,
			gpointer data);
static void on_menu_copy(gpointer data);
static void on_menu_paste(gpointer callback_data,
			guint callback_action,
			GtkWidget  *widget);

static void clipboard_receive(GtkClipboard *clipboard,
				const gchar *text,
				gpointer data);

GType timeentry_get_type(void)
{
	static GType te_type = 0;

	if (!te_type) {
		const GTypeInfo te_info =
			{
				sizeof (TimeentryClass),
				NULL, /* base_init */
				NULL, /* base_finalize */
				(GClassInitFunc) timeentry_class_init,
				NULL, /* class_finalize */
				NULL, /* class_data */
				sizeof (Timeentry),
				0,    /* n_preallocs */
				(GInstanceInitFunc) timeentry_init,
			};

		te_type = g_type_register_static (GTK_TYPE_HBOX,
						"Timeentry",
						&te_info,
						0);
	}

	return te_type;
}

static void timeentry_class_init(TimeentryClass *klass)
{
	timeentry_signals[SIGNAL_TIME_CHANGED] = g_signal_new ("time-changed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
				G_STRUCT_OFFSET (TimeentryClass, timeentry),
				NULL,
				NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
}

static void timeentry_init(Timeentry *timeentry)
{

	/* Set default minmax */
	timeentry->min_seconds = 0;
	timeentry->min_nanoseconds = 0;
	timeentry->max_seconds = 1;
	timeentry->max_nanoseconds = 1;

	/* Add main label*/
	timeentry->main_label = gtk_label_new(NULL);
	gtk_widget_show(timeentry->main_label);

	timeentry->main_label_box = gtk_event_box_new();
	gtk_widget_show(timeentry->main_label_box);
	gtk_container_add(GTK_CONTAINER(timeentry->main_label_box), timeentry->main_label);

	gtk_widget_set_tooltip_text(timeentry->main_label_box, "Paste time here");

	/* Add seconds spinner */
	timeentry->seconds_spinner = gtk_spin_button_new_with_range(timeentry->min_seconds,
								timeentry->max_seconds,
								1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(timeentry->seconds_spinner), 0);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(timeentry->seconds_spinner), TRUE);
	gtk_widget_show(timeentry->seconds_spinner);

	/* Add nanoseconds spinner */
	/* TODO ybrosseau 2010-11-24: Add wrap management */
	timeentry->nanoseconds_spinner = gtk_spin_button_new_with_range(timeentry->min_nanoseconds,
									timeentry->max_nanoseconds,
									1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(timeentry->nanoseconds_spinner), 0);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(timeentry->nanoseconds_spinner), TRUE);
	gtk_widget_show(timeentry->nanoseconds_spinner);

	/* s and ns labels */
	timeentry->s_label = gtk_label_new("s ");
	gtk_widget_show(timeentry->s_label);
	timeentry->ns_label = gtk_label_new("ns ");
	gtk_widget_show(timeentry->ns_label);

	/* Pack everything */
	gtk_box_pack_start (GTK_BOX (timeentry), timeentry->main_label_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (timeentry), timeentry->seconds_spinner, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (timeentry), timeentry->s_label, FALSE, FALSE, 1);
	gtk_box_pack_start (GTK_BOX (timeentry), timeentry->nanoseconds_spinner, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (timeentry), timeentry->ns_label, FALSE, FALSE, 1);

	timeentry->seconds_changed_handler_id =
		g_signal_connect ((gpointer) timeentry->seconds_spinner, "value-changed",
				G_CALLBACK (on_spinner_value_changed),
				timeentry);

	timeentry->nanoseconds_changed_handler_id =
		g_signal_connect ((gpointer) timeentry->nanoseconds_spinner, "value-changed",
				G_CALLBACK (on_spinner_value_changed),
				timeentry);

	/* Add pasting callbacks */
	g_signal_connect ((gpointer) timeentry->main_label_box, "button-press-event",
			G_CALLBACK (on_label_click),
			timeentry);

	/* Create pasting context-menu */
	GtkItemFactory *item_factory;
	/* Our menu, an array of GtkItemFactoryEntry structures that defines each menu item */
	GtkItemFactoryEntry menu_items[] = {
		{ "/Copy time",      NULL, on_menu_copy,    0, "<Item>" },
		{ "/Paste time",     NULL, on_menu_paste,   0, "<Item>" },
	};

	gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

	item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main_label>",
					NULL);
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, timeentry);
	timeentry->main_label_context_menu = gtk_item_factory_get_widget (item_factory, "<main_label>");
}

void timeentry_set_main_label (Timeentry *timeentry,
			const gchar *str)
{
	g_return_if_fail (IS_TIMEENTRY (timeentry));

	g_object_freeze_notify (G_OBJECT (timeentry));

	gtk_label_set_label(GTK_LABEL(timeentry->main_label), str);

	g_object_thaw_notify (G_OBJECT (timeentry));
}

static void timeentry_update_nanoseconds_spinner_range(Timeentry *timeentry,
						unsigned long current_seconds)
{
	if (current_seconds > timeentry->min_seconds && current_seconds < timeentry->max_seconds) {
		/* We are not at a limit, set the spinner to full range */
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(timeentry->nanoseconds_spinner),
					0,
					MAX_NANOSECONDS);
	} else if (timeentry->min_seconds == timeentry->max_seconds) {
		/* special case were the time span is less than a second */
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(timeentry->nanoseconds_spinner),
					timeentry->min_nanoseconds,
					timeentry->max_nanoseconds);

	} else if (current_seconds <= timeentry->min_seconds) {
		/* We are a the start limit */
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(timeentry->nanoseconds_spinner),
					timeentry->min_nanoseconds,
					MAX_NANOSECONDS);
	} else if (current_seconds >= timeentry->max_seconds) {
		/* We are a the stop limit */
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(timeentry->nanoseconds_spinner),
					0,
					timeentry->max_nanoseconds);
	} else {
		/* Should never happen */
		g_assert(FALSE);
	}
}

void timeentry_set_minmax_time(Timeentry *timeentry,
				unsigned long min_seconds,
				unsigned long min_nanoseconds,
				unsigned long max_seconds,
				unsigned long max_nanoseconds)
{
	unsigned long current_seconds;
	unsigned long current_nanoseconds;

	timeentry_get_time(timeentry, &current_seconds, &current_nanoseconds);

	if (min_seconds > max_seconds ||
		(min_seconds == max_seconds && min_nanoseconds > max_nanoseconds)) {
		return;
	}

	timeentry->min_seconds = min_seconds;
	timeentry->min_nanoseconds = min_nanoseconds;
	timeentry->max_seconds = max_seconds;
	timeentry->max_nanoseconds = max_nanoseconds;

	/* Disable the widgets if there is no range possible */
	if (min_seconds == max_seconds &&
		min_nanoseconds == max_nanoseconds) {
		gtk_widget_set_sensitive(timeentry->seconds_spinner, FALSE);
		gtk_widget_set_sensitive(timeentry->nanoseconds_spinner, FALSE);

	} else {
		gtk_widget_set_sensitive(timeentry->seconds_spinner, TRUE);
		gtk_widget_set_sensitive(timeentry->nanoseconds_spinner, TRUE);
	}

	/* Set the new time range */
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(timeentry->seconds_spinner),
				timeentry->min_seconds,
				timeentry->max_seconds);

	timeentry_update_nanoseconds_spinner_range(timeentry,
						current_seconds);

	/* Update time if necessary */
	timeentry_set_time(timeentry, current_seconds, current_nanoseconds);
}

void timeentry_set_time(Timeentry *timeentry,
			unsigned long seconds,
			unsigned long nanoseconds)
{
	/* Set the passed time in the valid range */
	if (seconds < timeentry->min_seconds) {
		seconds = timeentry->min_seconds;
		nanoseconds = timeentry->min_nanoseconds;

	}
	if (seconds == timeentry->min_seconds &&
		nanoseconds < timeentry->min_nanoseconds) {
		nanoseconds = timeentry->min_nanoseconds;
	}
	if (seconds > timeentry->max_seconds) {
		seconds = timeentry->max_seconds;
		nanoseconds = timeentry->max_nanoseconds;
	}
	if (seconds == timeentry->max_seconds &&
		nanoseconds > timeentry->max_nanoseconds) {
		nanoseconds = timeentry->max_nanoseconds;
	}

	if ((gtk_spin_button_get_value (GTK_SPIN_BUTTON(timeentry->seconds_spinner)) == seconds) &&
		(gtk_spin_button_get_value (GTK_SPIN_BUTTON(timeentry->nanoseconds_spinner)) == nanoseconds)) {
		/* No update needed, don't update the spinners */
		return;
	}

	/* Block the spinner changed signal when we set the time to them */
	g_signal_handler_block(timeentry->seconds_spinner,
			       timeentry->seconds_changed_handler_id);
	g_signal_handler_block(timeentry->nanoseconds_spinner,
			       timeentry->nanoseconds_changed_handler_id);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(timeentry->seconds_spinner), seconds);
	timeentry_update_nanoseconds_spinner_range(timeentry, seconds);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(timeentry->nanoseconds_spinner), nanoseconds);

	g_signal_handler_unblock(timeentry->nanoseconds_spinner,
				 timeentry->nanoseconds_changed_handler_id);
	g_signal_handler_unblock(timeentry->seconds_spinner,
				 timeentry->seconds_changed_handler_id);

	/* Send the time changed signal */
	g_signal_emit(timeentry,
		timeentry_signals[SIGNAL_TIME_CHANGED], 0);
}

void timeentry_get_time (Timeentry *timeentry,
			unsigned long *seconds,
			unsigned long *nanoseconds)
{
	*seconds = gtk_spin_button_get_value (GTK_SPIN_BUTTON(timeentry->seconds_spinner));
	*nanoseconds = gtk_spin_button_get_value (GTK_SPIN_BUTTON(timeentry->nanoseconds_spinner));
}

static void
on_spinner_value_changed (GtkSpinButton *spinbutton,
			gpointer user_data)
{
	Timeentry *timeentry = (Timeentry *)user_data;
	unsigned long current_seconds;

	/* Manage min/max values of the nanoseconds spinner */
	current_seconds = gtk_spin_button_get_value (GTK_SPIN_BUTTON(timeentry->seconds_spinner));
	timeentry_update_nanoseconds_spinner_range(timeentry,
						current_seconds);

	g_signal_emit(timeentry,
		timeentry_signals[SIGNAL_TIME_CHANGED], 0);
}

static gboolean on_label_click(GtkWidget *widget,
			GdkEventButton *event,
			gpointer data)
{
	Timeentry *timeentry = (Timeentry *)data;

	/* Only take button presses */
	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;


	if (event->button == 3) {
		/* Right button click - popup menu */

		/* Show the menu */
		gtk_menu_popup (GTK_MENU(timeentry->main_label_context_menu), NULL, NULL,
				NULL, NULL, event->button, event->time);

		return TRUE;

	} else if (event->button == 2) {
		/* Middle button click - paste PRIMARY */

		GtkClipboard *clip = gtk_clipboard_get_for_display(gdk_display_get_default(),
								GDK_SELECTION_PRIMARY);
		gtk_clipboard_request_text(clip,
					(GtkClipboardTextReceivedFunc)clipboard_receive,
					(gpointer)timeentry);
	}

	return 0;
}

static void on_menu_copy(gpointer callback_data)
{
	Timeentry *timeentry = (Timeentry *)callback_data;
	const int CLIP_BUFFER_SIZE = 100;
	gchar buffer[CLIP_BUFFER_SIZE];

	unsigned long seconds, nseconds;
	timeentry_get_time(timeentry, &seconds, &nseconds);
	snprintf(buffer, CLIP_BUFFER_SIZE, "%lu.%lu", seconds, nseconds);

	/* Set the CLIPBOARD */
	GtkClipboard *clip = gtk_clipboard_get_for_display(gdk_display_get_default(),
							GDK_SELECTION_CLIPBOARD);

	gtk_clipboard_set_text(clip, buffer, -1);

	/* Set it also in the PRIMARY buffer (for middle click) */
	clip = gtk_clipboard_get_for_display(gdk_display_get_default(),
					GDK_SELECTION_PRIMARY);
	gtk_clipboard_set_text(clip, buffer, -1);
}

static void on_menu_paste(gpointer callback_data,
			guint callback_action,
			GtkWidget *widget) 
{
	Timeentry *timeentry = (Timeentry *)callback_data;

	GtkClipboard *clip = gtk_clipboard_get_for_display(gdk_display_get_default(),
							GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_request_text(clip,
				(GtkClipboardTextReceivedFunc)clipboard_receive,
				(gpointer)timeentry);
}

static void clipboard_receive(GtkClipboard *clipboard,
			const gchar *text,
			gpointer data)
{
	const int CLIP_BUFFER_SIZE = 100;
	if (text == NULL) {
		return;
	}
	Timeentry *timeentry = (Timeentry *)data;
	gchar buffer[CLIP_BUFFER_SIZE];
	gchar *ptr = buffer, *ptr_sec, *ptr_nsec;

	strncpy(buffer, text, CLIP_BUFFER_SIZE);
	g_debug("Timeentry clipboard receive: %s", buffer);

	while (!isdigit(*ptr) && ptr < buffer+CLIP_BUFFER_SIZE-1) {
		ptr++;
	}
	/* remove leading junk */
	ptr_sec = ptr;
	while (isdigit(*ptr) && ptr < buffer+CLIP_BUFFER_SIZE-1) {
		ptr++;
	}
	/* read all the first number */
	*ptr = '\0';

	if (ptr == ptr_sec) {
		/* No digit in the input, exit */
		return;
	}
	ptr++;

	while (!isdigit(*ptr) && ptr < buffer+CLIP_BUFFER_SIZE-1) {
		ptr++;
	}
	/* remove leading junk */
	ptr_nsec = ptr;
	while (isdigit(*ptr) && ptr < buffer+CLIP_BUFFER_SIZE-1) {
		ptr++;
	}
	/* read all the first number */
	*ptr = '\0';

	timeentry_set_time(timeentry,
			strtoul(ptr_sec, NULL, 10),
			strtoul(ptr_nsec, NULL, 10));
}

GtkWidget*
timeentry_new (const gchar *label)
{

	Timeentry *timeentry = g_object_new (TIMEENTRY_TYPE, NULL);

	if (label && *label)
		timeentry_set_main_label (timeentry, label);

	return GTK_WIDGET(timeentry);
}
