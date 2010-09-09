/**
 * collection4 - action_show_instance.c
 * Copyright (C) 2010  Florian octo Forster
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Authors:
 *   Florian octo Forster <ff at octo.it>
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

#include "action_show_instance.h"
#include "common.h"
#include "graph.h"
#include "graph_ident.h"
#include "graph_instance.h"
#include "graph_list.h"
#include "utils_cgi.h"

#include <fcgiapp.h>
#include <fcgi_stdio.h>

#define OUTPUT_ERROR(...) do {             \
  printf ("Content-Type: text/plain\n\n"); \
  printf (__VA_ARGS__);                    \
  return (0);                              \
} while (0)

#define MAX_SHOW_GRAPHS 10

struct show_graph_data_s
{
  graph_config_t *cfg;
  graph_instance_t *inst;
  int graph_count;
};
typedef struct show_graph_data_s show_graph_data_t;

static void show_breadcrump_field (const char *str, /* {{{ */
    const char *field_name)
{
  if ((str == NULL) || (str[0] == 0))
    printf ("<em>none</em>");
  else if (IS_ANY (str))
    printf ("<em>any</em>");
  else if (IS_ALL (str))
    printf ("<em>all</em>");
  else
  {
    char *str_html = html_escape (str);

    if (field_name != NULL)
      printf ("<a href=\"%s?action=search;q=%s:%s\">%s</a>",
          script_name (), field_name, str_html, str_html);
    else
      printf ("<a href=\"%s?action=search;q=%s\">%s</a>",
          script_name (), str_html, str_html);

    free (str_html);
  }
} /* }}} void show_breadcrump_field */

static int show_breadcrump (graph_config_t *cfg, /* {{{ */
    graph_instance_t *inst)
{
  graph_ident_t *ident;
  char *prefix;

  if (inst != NULL)
  {
    prefix = "Instance";
    ident = inst_get_selector (inst);
  }
  else
  {
    prefix = "Graph";
    ident = graph_get_selector (cfg);
  }

  printf ("<div class=\"breadcrump\">%s: &quot;", prefix);
  show_breadcrump_field (ident_get_host (ident), "host");
  printf ("&nbsp;/ ");
  show_breadcrump_field (ident_get_plugin (ident), "plugin");
  printf ("&nbsp;&ndash; ");
  show_breadcrump_field (ident_get_plugin_instance (ident), "plugin_instance");
  printf ("&nbsp;/ ");
  show_breadcrump_field (ident_get_type (ident), "type");
  printf ("&nbsp;&ndash; ");
  show_breadcrump_field (ident_get_type_instance (ident), "type_instance");
  printf ("&quot;</div>\n");

  ident_destroy (ident);
  return (0);
} /* }}} int show_breadcrump */

static int show_time_selector (__attribute__((unused)) void *user_data) /* {{{ */
{
  param_list_t *pl;

  pl = param_create (/* query string = */ NULL);
  param_set (pl, "begin", NULL);
  param_set (pl, "end", NULL);
  param_set (pl, "button", NULL);

  printf ("<form action=\"%s\" method=\"get\">\n", script_name ());

  param_print_hidden (pl);

  printf ("  <select name=\"begin\">\n"
      "    <option value=\"-3600\">Hour</option>\n"
      "    <option value=\"-86400\">Day</option>\n"
      "    <option value=\"-604800\">Week</option>\n"
      "    <option value=\"-2678400\">Month</option>\n"
      "    <option value=\"-31622400\">Year</option>\n"
      "  </select>\n"
      "  <input type=\"submit\" name=\"button\" value=\"Go\" />\n");

  printf ("</form>\n");

  param_destroy (pl);

  return (0);
} /* }}} int show_time_selector */

static int left_menu (void *user_data) /* {{{ */
{
  show_graph_data_t *data = user_data;
  char params[1024];
  graph_instance_t *inst;
  graph_ident_t *ident;
  const char *host;

  memset (params, 0, sizeof (params));
  graph_get_params (data->cfg, params, sizeof (params));
  html_escape_buffer (params, sizeof (params));

  inst = inst_get_selected (data->cfg);
  ident = inst_get_selector (inst);
  host = ident_get_host (ident);
  if (IS_ANY (host))
    host = NULL;

  printf ("\n<ul class=\"menu left\">\n"
      "  <li><a href=\"%s?action=show_graph;%s\">All instances</a></li>\n"
      "  <li><a href=\"%s?action=list_graphs\">All graphs</a></li>\n",
      script_name (), params,
      script_name ());
  if (host != NULL)
  {
    char host_html[1024];
    char host_uri[1024];

    html_escape_copy (host_html, host, sizeof (host_html));
    uri_escape_copy (host_uri, host, sizeof (host_uri));

    printf ("  <li><a href=\"%s?action=search;q=host:%s\">Host &quot;%s&quot;</a></li>\n",
        script_name (), host_uri, host_html);
  }
  printf ("</ul>\n");

  host = NULL;
  ident_destroy (ident);

  return (0);
} /* }}} int left_menu */

static int show_instance_cb (graph_config_t *cfg, /* {{{ */
    graph_instance_t *inst,
    void *user_data)
{
  show_graph_data_t *data = user_data;
  char title[128];
  char descr[128];
  char params[1024];

  long begin;
  long end;
  char time_params[128];
  int status;

  memset (title, 0, sizeof (title));
  graph_get_title (cfg, title, sizeof (title));
  html_escape_buffer (title, sizeof (title));

  memset (descr, 0, sizeof (descr));
  inst_describe (cfg, inst, descr, sizeof (descr));
  html_escape_buffer (descr, sizeof (descr));

  memset (params, 0, sizeof (params));
  inst_get_params (cfg, inst, params, sizeof (params));
  html_escape_buffer (params, sizeof (params));

  time_params[0] = 0;
  begin = 0;
  end = 0;

  status = get_time_args (&begin, &end, /* now = */ NULL);
  if (status == 0)
  {
    snprintf (time_params, sizeof (time_params), ";begin=%li;end=%li",
        begin, end);
    time_params[sizeof (time_params) - 1] = 0;
  }

  printf ("<h2>Instance &quot;%s&quot;</h2>\n", descr);

  show_breadcrump (cfg, inst);

  if (data->graph_count < MAX_SHOW_GRAPHS)
  {
    printf ("<div class=\"graph-img\"><img src=\"%s?action=graph;%s%s\" "
        "title=\"%s / %s\" /></div>\n",
        script_name (), params, time_params, title, descr);
    printf ("<div style=\"clear: both;\"><a href=\"%s?action=graph_data_json;%s%s\">"
        "Get graph data as JSON</a></div>\n",
        script_name (), params, time_params);
  }
  else
    printf ("<a href=\"%s?action=show_instance;%s\">Show graph "
        "&quot;%s / %s&quot;</a>\n",
        script_name (), params, title, descr);

  data->graph_count++;

  return (0);
} /* }}} int show_instance_cb */

static int show_instance (void *user_data) /* {{{ */
{
  show_graph_data_t *data = user_data;
  char params[1024];
  int status;

  status = inst_get_all_selected (data->cfg,
      /* callback = */ show_instance_cb, /* user data = */ data);
  if (status != 0)
    fprintf (stderr, "show_instance: inst_get_all_selected failed "
        "with status %i\n", status);

  memset (params, 0, sizeof (params));
  graph_get_params (data->cfg, params, sizeof (params));
  html_escape_buffer (params, sizeof (params));

  printf ("<div style=\"clear: both;\"><a href=\"%s?action=graph_def_json;%s\">"
      "Get graph definition as JSON</a></div>\n",
      script_name (), params);

  return (0);
} /* }}} int show_instance */

int action_show_instance (void) /* {{{ */
{
  page_callbacks_t pg_callbacks = PAGE_CALLBACKS_INIT;
  show_graph_data_t pg_data;

  char tmp[128];
  char title[128];

  memset (&pg_data, 0, sizeof (pg_data));
  pg_data.cfg = gl_graph_get_selected ();
  if (pg_data.cfg == NULL)
    OUTPUT_ERROR ("gl_graph_get_selected () failed.\n");

  memset (tmp, 0, sizeof (tmp));
  graph_get_title (pg_data.cfg, tmp, sizeof (tmp));
  snprintf (title, sizeof (title), "Graph \"%s\"", tmp);
  title[sizeof (title) - 1] = 0;

  pg_callbacks.top_right = html_print_search_box;
  pg_callbacks.middle_center = show_instance;
  pg_callbacks.middle_left = left_menu;
  pg_callbacks.middle_right = show_time_selector;

  html_print_page (title, &pg_callbacks, &pg_data);

  return (0);
} /* }}} int action_show_instance */

/* vim: set sw=2 sts=2 et fdm=marker : */
