/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
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

/*
   consist in AND, OR and NOT nested expressions, forming a tree with 
   simple relations as leaves. The simple relations test is a field
   in an event is equal, not equal, smaller, smaller or equal, larger, or
   larger or equal to a specified value. 
*/

#include <lttv/filter.h>

/*
  read_token

  read_expression
    ( read expr )
    simple expr [ op expr ]

  read_simple_expression
    read_field_path [ rel value ]

  read_field_path
    read_field_component [. field path]

  read_field_component
    name [ \[ value \] ]

  data struct:
  and/or(left/right)
  not(child)
  op(left/right)
  path(component...) -> field
*/

/**
 * 	Add an filtering option to the current tree
 * 	@param expression Current expression to parse
 * 	@return success/failure of operation
 */
gboolean
parse_simple_expression(GString* expression) {
	
	unsigned i;

  

}

/**
 * 	Creates a new lttv_filter
 * 	@param expression filtering options string
 * 	@param t pointer to the current LttvTrace
 * 	@return the current lttv_filter or NULL if error
 */
lttv_filter*
lttv_filter_new(char *expression, LttvTraceState *tfs) {

  /* test */
  char* field = "cpu";
  LttEventType *et;
  
 // LttField* a_ltt_field = NULL;
 // a_ltt_field = find_field(NULL,field);

 // g_print("%s\n",a_ltt_field->field_type->type_name);

  return NULL;
  
  unsigned 	
    i, 
    p=0,	/* parenthesis nesting value */
    b=0;	/* current breakpoint in expression string */
	
  gpointer tree = NULL;
  
  /* temporary values */
  GString *current_option = g_string_new(""); 
  lttv_simple_expression a_simple_expression;

  g_print("filter::lttv_filter_new()\n");		/* debug */

  /*	
   *  1. parse expression
   *  2. construct binary tree
   *  3. return corresponding filter
   */

  /*
   * 	Binary tree memory allocation
   * 	- based upon a preliminary block size
   */
  gulong size = (strlen(expression)/AVERAGE_EXPRESSION_LENGTH)*MAX_FACTOR;
  tree = g_malloc(size*sizeof(lttv_filter_tree));
    
  /*
   *	Parse entire expression and construct
   *	the binary tree.  There are two steps 
   *	in browsing that string
   *	  1. finding boolean ops ( &,|,^,! ) and parenthesis
   *	  2. finding simple expressions
   *	    - field path
   *	    - op ( >, <, =, >=, <=, !=)
   *	    - value
   */

  for(i=0;i<strlen(expression);i++) {
    g_print("%s\n",current_option->str);
    switch(expression[i]) {
      /*
       *   logical operators
       */
      case '&':   /* and */
      case '|':   /* or */
      case '^':   /* xor */
        current_option = g_string_new("");
        break;
      case '!':   /* not, or not equal (math op) */
        if(expression[i+1] == '=') {  /* != */
          a_simple_expression.op = LTTV_FIELD_NE;
          i++;
        } else {  /* ! */
          g_print("%s\n",current_option);
          current_option = g_string_new("");
        }
        break;
      case '(':   /* start of parenthesis */
        p++;      /* incrementing parenthesis nesting value */
        break;
      case ')':   /* end of parenthesis */
        p--;      /* decrementing parenthesis nesting value */
        break;

      /*	
       *	mathematic operators
       */
      case '<':   /* lower, lower or equal */
        if(expression[i+1] == '=') { /* <= */
          i++;
          a_simple_expression.op = LTTV_FIELD_LE;                  
        } else a_simple_expression.op = LTTV_FIELD_LT;
        break;
      case '>':   /* higher, higher or equal */
        if(expression[i+1] == '=') {  /* >= */
          i++;
          a_simple_expression.op = LTTV_FIELD_GE;                  
        } else a_simple_expression.op = LTTV_FIELD_GT;
        break;
      case '=':   /* equal */
        a_simple_expression.op = LTTV_FIELD_EQ;
        break;
      default:    /* concatening current string */
        g_string_append_c(current_option,expression[i]); 				
    }
  }


  
  if( p>0 ) { 
    g_warning("Wrong filtering options, the string\n\"%s\"\n\
        is not valid due to parenthesis incorrect use",expression);	
    return NULL;
  }
}

/**
 * 	Apply the filter to a specific trace
 * 	@param filter the current filter applied
 * 	@param tracefile the trace to apply the filter to
 * 	@return success/failure of operation
 */
gboolean
lttv_filter_tracefile(lttv_filter *filter, LttvTrace *tracefile) {

}

/**
 * 	Apply the filter to a specific event
 * 	@param filter the current filter applied
 * 	@param event the event to apply the filter to
 * 	@return success/failure of operation
 */
gboolean
lttv_filter_event(lttv_filter *filter, LttEvent *event) {

}
