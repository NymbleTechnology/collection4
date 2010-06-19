#ifndef GRAPH_LIST_H
#define GRAPH_LIST_H 1

#include "graph_types.h"
#include "graph.h"
#include "graph_instance.h"

/*
 * Functions
 */
int gl_add_graph (graph_config_t *cfg);

int gl_config_submit (void);

graph_config_t *gl_graph_get_selected (void);

int gl_graph_get_all (graph_callback_t callback, void *user_data);

int gl_graph_instance_get_all (graph_config_t *cfg, graph_inst_callback_t callback,
    void *user_data);

int gl_instance_get_all (graph_inst_callback_t callback, void *user_data);

int gl_search (const char *search, graph_inst_callback_t callback,
    void *user_data);

int gl_update (void);

#endif /* GRAPH_LIST_H */
/* vim: set sw=2 sts=2 et fdm=marker : */