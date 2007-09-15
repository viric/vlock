#include <stdlib.h>
#include <string.h>

#include "list.h"

#include "plugin.h"
#include "util.h"

struct list *dependency_names;

void __attribute__((constructor)) init_dependency_names(void)
{
  dependency_names = list_new();
  list_append(dependency_names, "after");
  list_append(dependency_names, "before");
  list_append(dependency_names, "requires");
  list_append(dependency_names, "needs");
  list_append(dependency_names, "depends");
  list_append(dependency_names, "conflicts");
}

void __attribute__((destructor)) uninit_dependency_names(void)
{
  list_free(dependency_names);
}

struct list *hook_names;

void __attribute__((constructor)) init_hook_names(void)
{
  hook_names = list_new();
  list_append(hook_names, "vlock_start");
  list_append(hook_names, "vlock_end");
  list_append(hook_names, "vlock_save");
  list_append(hook_names, "vlock_save_abort");
}

void __attribute__((destructor)) uninit_hook_names(void)
{
  list_free(hook_names);
}

struct plugin *__allocate_plugin(const char *name)
{
  struct plugin *p = ensure_malloc(sizeof *p);
  p->name = strdup(name);
  p->dependencies = list_new();

  list_for_each(dependency_names, name_item)
    list_append(p->dependencies, list_new());

  return p;
}

void __destroy_plugin(struct plugin *p)
{
  list_for_each(p->dependencies, dependency_list_item) {
    struct list *dependency_list = dependency_list_item->data;

    list_for_each(dependency_list, dependency_item)
      free(dependency_item->data);

    list_free(dependency_list);
  }

  list_free(p->dependencies);
  free(p->name);
  free(p);
}

void destroy_plugin(struct plugin *p)
{
  p->close(p);
  __destroy_plugin(p);
}
