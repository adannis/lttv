#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <lttv/common.h>

/* constructor of the viewer */
//typedef GtkWidget* (*lttv_constructor)(void * main_window);


typedef GArray LttvToolbars;

typedef struct _lttv_toolbar_closure {
  lttv_constructor con;
  char * tooltip;
  char ** pixmap;
} lttv_toolbar_closure;

LttvToolbars *lttv_toolbars_new();

void lttv_toolbars_destroy(LttvToolbars *h);

void lttv_toolbars_add(LttvToolbars *h, lttv_constructor f, char* tooltip, char ** pixmap);

gboolean lttv_toolbars_remove(LttvToolbars *h, lttv_constructor f);

unsigned lttv_toolbars_number(LttvToolbars *h);

#endif // TOOLBAR_H
