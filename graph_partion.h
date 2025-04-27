#ifndef GRAPH_PARTION_H
#define GRAPH_PARTION_H

#include <stddef.h>
#include <metis.h>

typedef struct {
    int max_neighbors;
    int *adjncy;
    int *xadj;
    int nvtxs;
    int *components;
    int *component_ptr;
    int num_components;
} Graph;

idx_t* Graph_parts(Graph* Origin_Graph, int partions_count, float error_margin, int* deleted_edges);

Graph** graph_partition(Graph* Origin_Graph, idx_t* parts, int partions, float error_margin);

int partition_graph_and_save(Graph* input_graph, int partions_count, float error_margin, const char* output_format);

void print_graph_info(Graph* graph, const char* name);

#endif

