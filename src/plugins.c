#include <stdio.h>
#include <stdlib.h>

#include "plugins.h"

#include "list.h"

#include "plugin.h"
#include "module.h"
#include "script.h"
#include "util.h"

const char *dependency_names[nr_dependencies] = {
  "after",
  "before",
  "requires",
  "needs",
  "depends",
  "conflicts",
};

static void handle_vlock_start(const char * hook_name);
static void handle_vlock_end(const char * hook_name);
static void handle_vlock_save(const char * hook_name);
static void handle_vlock_save_abort(const char * hook_name);

const struct hook hooks[nr_hooks] = {
  { "vlock_start", handle_vlock_start },
  { "vlock_end", handle_vlock_end },
  { "vlock_save", handle_vlock_save },
  { "vlock_save_abort", handle_vlock_save_abort },
};

static struct list *plugins;


static struct plugin *get_plugin(const char *name)
{
  return NULL;
}

void load_plugin(const char *name)
{
  char *e1 = NULL;
  char *e2 = NULL;
  struct plugin *p = get_plugin(name);

  if (p != NULL)
    return;

  p = open_module(name, &e1);

  if (p == NULL)
    p = open_script(name, &e2);

  if (p == NULL) {
    if (e1 == NULL && e2 == NULL)
      fatal_error("vlock-plugins: error loading plugin '%s'\n", name);

    if (e1 != NULL) {
      fprintf(stderr, "vlock-plugins: error loading module '%s': %s\n", name, e1);
      free(e1);
    }
    if (e2 != NULL) {
      fprintf(stderr, "vlock-plugins: error loading script '%s': %s\n", name, e2);
      free(e2);
    }

    abort();
  }

  list_append(plugins, p);
}

static void __resolve_depedencies(void)
{
}

static void sort_plugins(void)
{
}

void resolve_dependencies(void)
{
  __resolve_depedencies();
  sort_plugins();
}

void unload_plugins(void)
{
}

void plugin_hook(const char *hook_name)
{
}

void handle_vlock_start(const char *hook_name)
{
}

void handle_vlock_end(const char *hook_name)
{
}

void handle_vlock_save(const char *hook_name)
{
}

void handle_vlock_save_abort(const char *hook_name)
{
}
