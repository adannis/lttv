/******************************************************************************
 * Draw_Item.c
 *
 * This file contains methods responsible for drawing a generic type of data
 * in a drawable. Doing this generically will permit user defined drawing
 * behavior in a later time.
 *
 * This file provides an API which is meant to be reusable for all viewers that
 * need to show information in line, icon, text, background or point form in
 * a drawable area having time for x axis. The y axis, in the control flow
 * viewer case, is corresponding to the different processes, but it can be
 * reused integrally for cpu, and eventually locks, buffers, network
 * interfaces... What will differ between the viewers is the precise
 * information which interests us. We may think that the most useful
 * information for control flow are some specific events, like schedule
 * change, and processes'states. It may differ for a cpu viewer : the
 * interesting information could be more the execution mode of each cpu.
 * This API in meant to make viewer's writers life easier : it will become
 * a simple choice of icons and line types for the precise information
 * the viewer has to provide (agremented with keeping supplementary records
 * and modifying slightly the DrawContext to suit the needs.)
 *
 * We keep each data type in attributes, keys to specific information
 * being formed from the GQuark corresponding to the information received.
 * (facilities / facility_name / events / eventname.)
 * (cpus/cpu_name, process_states/ps_name,
 * execution_modes/em_name, execution_submodes/es_name).
 * The goal is then to provide a generic way to print information on the
 * screen for all this different information.
 *
 * Information can be printed as
 *
 * - text (text + color + size + position (over or under line)
 * - icon (icon filename, corresponding to a loaded icon, accessible through
 *   a GQuark. Icons are loaded statically at the guiControlFlow level during
 *   module initialization and can be added on the fly if not present in the
 *   GQuark.) The habitual place for xpm icons is in
 *   ${prefix}/share/LinuxTraceToolkit.) + position (over or under line)
 * - line (color, width, style)
 * - Arc (can be seen as points) (color, size)
 * - background color (color)
 *
 * Each item has an array of hooks (hook list). Each hook represents an
 * operation to perform. We seek the array each time we want to
 * draw an item. We execute each operation in order. An operation type
 * is associated with each hook to permit user listing and modification
 * of these operations. The operation type is also used to find the
 * corresponding priority for the sorting. Operation type and priorities
 * are enum and a static int table.
 *
 * The array has to be sorted by priority each time we add a task in it.
 * A priority is associated with each operation type. It permits
 * to perform background color selection before line or text drawing. We also
 * draw lines before text, so the text appears over the lines.
 *
 * Executing all the arrays of operations for a specific event (which
 * implies information for state, event, cpu, execution mode and submode)
 * has to be done in a same DrawContext. The goal there is to keep the offset
 * of the text and icons over and under the middle line, so a specific
 * event could be printed as (  R Si 0 for running, scheduled in, cpu 0  ),
 * text being easy to replace with icons. The DrawContext is passed as
 * call_data for the operation hooks.
 *
 * We use the lttv global attributes to keep track of the loaded icons.
 * If we need an icon, we look for it in the icons / icon name pathname.
 * If found, we use the pointer to it. If not, we load the pixmap in
 * memory and set the pointer to the GdkPixmap in the attributes.
 * 
 * Author : Mathieu Desnoyers, October 2003
 */

#include <glib.h>
#include <lttv/hook.h>
#include <lttv/attribute.h>
#include <lttv/iattribute.h>

#include <lttv/processTrace.h>
#include <lttv/state.h>

/* The DrawContext keeps information about the current drawing position and
 * the previous one, so we can use both to draw lines.
 *
 * over : position for drawing over the middle line.
 * middle : middle line position.
 * under : position for drawing under the middle line.
 */
struct _DrawContext {
	GdkDrawable	*drawable;
	GdkGC		*gc;

	DrawInfo	*Current;
	DrawInfo	*Previous;
};

struct _DrawInfo {
	ItemInfo	*over;
	ItemInfo	*middle;
	ItemInfo	*under;
};

/* LttvExecutionState is accessible through the LttvTracefileState. Is has
 * a pointer to the LttvProcessState which points to the top of stack
 * execution state : LttvExecutionState *state.
 *
 * LttvExecutionState contains (useful here):
 * LttvExecutionMode t,
 * LttvExecutionSubmode n,
 * LttvProcessStatus s
 * 
 *
 * LttvTraceState will be used in the case we need the string of the
 * different processes, eventtype_names, syscall_names, trap_names, irq_names.
 *
 * LttvTracefileState also gives the cpu_name and, as it herits from
 * LttvTracefileContext, it gives the LttEvent structure, which is needed
 * to get facility name and event name.
 */
struct _ItemInfo {
	gint	x, y;
	LttvTraceState		*ts;
	LttvTracefileState	*tfs;
};


/*
 * The Item element is only used so the DrawOperation is modifiable by users.
 * During drawing, only the Hook is needed.
 */
struct _DrawOperation {
	DrawableItems	Item;
	LttvHooks	*Hook;
};

/*
 * We define here each items that can be drawn, together with their
 * associated priority. Many item types can have the same priority,
 * it's only used for quicksorting the operations when we add a new one
 * to the array of operations to perform. Lower priorities are executed
 * first. So, for example, we may want to give background color a value
 * of 10 while a line would have 20, so the background color, which
 * is in fact a rectangle, does not hide the line.
 */

typedef enum _DrawableItems {
	ITEM_TEXT, ITEM_ICON, ITEM_LINE, ITEM_POINT, ITEM_BACKGROUND
} DrawableItems;

static gchar * Items_Priorities = {
	50,	/* ITEM_TEXT */
	40,	/* ITEM_ICON */
	20,	/* ITEM_LINE */
	30,	/* ITEM_POINT */
	10	/* ITEM_BACKGROUND */
};

typedef enum _RelPos {
	OVER, MIDDLE, UNDER
} RelPos;

/*
 * Here are the different structures describing each item type that can be
 * drawn. They contain the information necessary to draw the item : not the
 * position (this is provided by the DrawContext), but the text, icon name,
 * line width, color; all the properties of the specific items.
 */

struct _PropertiesText {
	GdkColor	*foreground;
	GdkColor	*background;
	gint		size;
	gchar		*Text;
	RelPos		position;
};


struct _PropertiesIcon {
	gchar		*icon_name;
	gint		width;
	gint		height;
	RelPos		position;
};

struct _PropertiesLine {
	GdkColor	*color;
	gint		line_width;
	GdkLineStyle	style;
	RelPos		position;
};

struct _PropertiesArc {
	GdkColor	*color;
	gint		size;	/* We force circle by width = height */
	gboolean	filled;
	RelPos		position;
};

struct _PropertiesBG {
	GdkColor	*color;
};





/* Drawing hook functions */
gboolean draw_text( void *hook_data, void *call_data)
{
	PropertiesText *Properties = (PropertiesText*)hook_data;
	DrawContext *Draw_Context = (DrawContext*)call_data;
}

gboolean draw_icon( void *hook_data, void *call_data)
{
	PropertiesIcon *Properties = (PropertiesIcon*)hook_data;
	DrawContext *Draw_Context = (DrawContext*)call_data;

}

gboolean draw_line( void *hook_data, void *call_data)
{
	PropertiesLine *Properties = (PropertiesLine*)hook_data;
	DrawContext *Draw_Context = (DrawContext*)call_data;

}

gboolean draw_arc( void *hook_data, void *call_data)
{
	PropertiesArc *Properties = (PropertiesArc*)hook_data;
	DrawContext *Draw_Context = (DrawContext*)call_data;

}

gboolean draw_bg( void *hook_data, void *call_data)
{
	PropertiesBG *Properties = (PropertiesBG*)hook_data;
	DrawContext *Draw_Context = (DrawContext*)call_data;

}


