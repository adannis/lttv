
#include <lttv/hook.h>


typedef struct _LttvHookClosure {
  LttvHook hook;
  void *hook_data;
} LttvHookClosure;


LttvHooks *lttv_hooks_new() 
{
  return g_array_new(FALSE, FALSE, sizeof(LttvHookClosure));
}


void lttv_hooks_destroy(LttvHooks *h) 
{
  g_critical("lttv_hooks_destroy()");
  g_array_free(h, TRUE);
}


void lttv_hooks_add(LttvHooks *h, LttvHook f, void *hook_data) 
{
  LttvHookClosure c;

  if(h == NULL)g_error("Null hook added");

  c.hook = f;
  c.hook_data = hook_data;
  g_array_append_val(h,c);
}


void lttv_hooks_add_list(LttvHooks *h, LttvHooks *list) 
{
  guint i;

  if(list == NULL) return;
  for(i = 0 ; i < list->len; i++) {
    g_array_append_val(h,g_array_index(list, LttvHookClosure, i));
  }
}


void *lttv_hooks_remove(LttvHooks *h, LttvHook f)
{
  unsigned i;

  void *hook_data;

  LttvHookClosure *c;

  for(i = 0 ; i < h->len ; i++) {
    c = &g_array_index(h, LttvHookClosure, i);
    if(c->hook == f) {
      hook_data = c->hook_data;
      lttv_hooks_remove_by_position(h, i);
      return hook_data;
    }
  }
  return NULL;
}


void lttv_hooks_remove_data(LttvHooks *h, LttvHook f, void *hook_data)
{
  unsigned i;

  LttvHookClosure *c;

  for(i = 0 ; i < h->len ; i++) {
    c = &g_array_index(h, LttvHookClosure, i);
    if(c->hook == f && c->hook_data == hook_data) {
      lttv_hooks_remove_by_position(h, i);
      return;
    }
  }
}


void lttv_hooks_remove_list(LttvHooks *h, LttvHooks *list)
{
  guint i, j;

  LttvHookClosure *c, *c_list;

  if(list == NULL) return;
  for(i = 0, j = 0 ; i < h->len && j < list->len ;) {
    c = &g_array_index(h, LttvHookClosure, i);
    c_list = &g_array_index(list, LttvHookClosure, j);
    if(c->hook == c_list->hook && c->hook_data == c_list->hook_data) {
      lttv_hooks_remove_by_position(h, i);
      j++;
    }
    else i++;
  }

  /* Normally the hooks in h are ordered as in list. If this is not the case,
     try harder here. */

  if(j < list->len) {
    for(; j < list->len ; j++) {
      c_list = &g_array_index(list, LttvHookClosure, j);
      lttv_hooks_remove_data(h, c_list->hook, c_list->hook_data);
    }
  }
}


unsigned lttv_hooks_number(LttvHooks *h)
{
  return h->len;
}


void lttv_hooks_get(LttvHooks *h, unsigned i, LttvHook *f, void **hook_data)
{
  LttvHookClosure *c;

  c = &g_array_index(h, LttvHookClosure, i);
  *f = c->hook;
  *hook_data = c->hook_data;
}


void lttv_hooks_remove_by_position(LttvHooks *h, unsigned i)
{
  g_array_remove_index(h, i);
}


gboolean lttv_hooks_call(LttvHooks *h, void *call_data)
{
  gboolean ret, sum_ret = FALSE;

  LttvHookClosure *c;

  guint i;

  if(h != NULL) {
    for(i = 0 ; i < h->len ; i++) {
      c = &g_array_index(h, LttvHookClosure, i);
      ret = c->hook(c->hook_data,call_data);
      sum_ret = sum_ret || ret;
    }
  }
  return sum_ret;
}


gboolean lttv_hooks_call_check(LttvHooks *h, void *call_data)
{
  LttvHookClosure *c;

  guint i;

  for(i = 0 ; i < h->len ; i++) {
    c = &g_array_index(h, LttvHookClosure, i);
    if(c->hook(c->hook_data,call_data)) return TRUE;
  }
  return FALSE;
}


LttvHooksById *lttv_hooks_by_id_new() 
{
  return g_ptr_array_new();
}


void lttv_hooks_by_id_destroy(LttvHooksById *h) 
{
  guint i;

  for(i = 0 ; i < h->len ; i++) {
    if(h->pdata[i] != NULL) lttv_hooks_destroy((LttvHooks *)(h->pdata[i]));
  }
  g_ptr_array_free(h, TRUE);
}


LttvHooks *lttv_hooks_by_id_find(LttvHooksById *h, unsigned id)
{
  if(h->len <= id) g_ptr_array_set_size(h, id + 1);
  if(h->pdata[id] == NULL) h->pdata[id] = lttv_hooks_new();
  return h->pdata[id];
}


unsigned lttv_hooks_by_id_max_id(LttvHooksById *h)
{
  return h->len;
}


LttvHooks *lttv_hooks_by_id_get(LttvHooksById *h, unsigned id)
{
  if(id < h->len) return h->pdata[id];
  return NULL;
}


void lttv_hooks_by_id_remove(LttvHooksById *h, unsigned id)
{
  if(id < h->len && h->pdata[id] != NULL) {
    lttv_hooks_destroy((LttvHooks *)h->pdata[id]);
    h->pdata[id] = NULL;
  }
}

