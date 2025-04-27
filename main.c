#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph_partion.h"

#define MAX_LINE 1000000
#define TEXT_MODE 0
#define BINARY_MODE 1

// Function declarations
int* parse_section(char *line, int *size);
int detect_file_type(const char *filename);
Graph* read_graph_text(const char *filename);
Graph* read_graph_binary(const char *filename);
Graph* read_graph(const char *filename);
void write_graph_text(const char *filename, Graph *graph);
void write_graph_binary(const char *filename, Graph *graph);
void write_graph(const char *filename, Graph *graph, const char *format);
void free_graph(Graph *graph);
void print_usage(const char *program_name);

int* parse_section(char *line, int *size) {
    int *arr = NULL;
    int capacity = 0;
    char *token = strtok(line, ";");

    while (token) {
        if (*size >= capacity) {
            capacity = capacity ? capacity * 2 : 1;
            arr = realloc(arr, capacity * sizeof(int));
        }
        arr[(*size)++] = atoi(token);
        token = strtok(NULL, ";");
    }
    return arr;
}

int detect_file_type(const char *filename) {
    char *ext = strrchr(filename, '.');
    if (ext && strcmp(ext, ".bin") == 0) {
        return BINARY_MODE;
    }
    return TEXT_MODE;
}

Graph* read_graph_text(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return NULL;
    }

    Graph *graph = malloc(sizeof(Graph));
    char line[MAX_LINE];

    // Section 1: max_neighbors
    fgets(line, MAX_LINE, fp);
    graph->max_neighbors = atoi(line);

    // Section 2: adjncy
    int adjncy_size = 0;
    fgets(line, MAX_LINE, fp);
    graph->adjncy = parse_section(line, &adjncy_size);

    // Section 3: xadj
    int xadj_size = 0;
    fgets(line, MAX_LINE, fp);
    graph->xadj = parse_section(line, &xadj_size);
    graph->nvtxs = xadj_size - 1;

    // Section 4: components
    int components_size = 0;
    fgets(line, MAX_LINE, fp);
    graph->components = parse_section(line, &components_size);

    // Section 5: component_ptr
    int component_ptr_size = 0;
    fgets(line, MAX_LINE, fp);
    graph->component_ptr = parse_section(line, &component_ptr_size);
    graph->num_components = component_ptr_size - 1;

    fclose(fp);
    return graph;
}

Graph* read_graph_binary(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return NULL;
    }

    Graph *graph = malloc(sizeof(Graph));

    // Read max_neighbors
    if (fread(&graph->max_neighbors, sizeof(int), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to read max_neighbors from %s\n", filename);
        free(graph);
        fclose(fp);
        return NULL;
    }

    // Read xadj size (nvtxs + 1)
    int xadj_size;
    if (fread(&xadj_size, sizeof(int), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to read xadj_size from %s\n", filename);
        free(graph);
        fclose(fp);
        return NULL;
    }
    graph->nvtxs = xadj_size - 1;

    // Read adjncy size (from last element of xadj)
    int adjncy_size;
    if (fread(&adjncy_size, sizeof(int), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to read adjncy_size from %s\n", filename);
        free(graph);
        fclose(fp);
        return NULL;
    }

    // Read num_components
    if (fread(&graph->num_components, sizeof(int), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to read num_components from %s\n", filename);
        free(graph);
        fclose(fp);
        return NULL;
    }

    // Calculate component size
    int components_size;
    if (fread(&components_size, sizeof(int), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to read components_size from %s\n", filename);
        free(graph);
        fclose(fp);
        return NULL;
    }

    // Allocate memory for all arrays
    graph->xadj = malloc(xadj_size * sizeof(int));
    graph->adjncy = malloc(adjncy_size * sizeof(int));
    graph->component_ptr = malloc((graph->num_components + 1) * sizeof(int));
    graph->components = malloc(components_size * sizeof(int));

    // Read xadj array
    if (fread(graph->xadj, sizeof(int), xadj_size, fp) != xadj_size) {
        fprintf(stderr, "Error: Failed to read xadj from %s\n", filename);
        free_graph(graph);
        fclose(fp);
        return NULL;
    }

    // Read adjncy array
    if (fread(graph->adjncy, sizeof(int), adjncy_size, fp) != adjncy_size) {
        fprintf(stderr, "Error: Failed to read adjncy from %s\n", filename);
        free_graph(graph);
        fclose(fp);
        return NULL;
    }

    // Read component_ptr array
    if (fread(graph->component_ptr, sizeof(int), graph->num_components + 1, fp) != graph->num_components + 1) {
        fprintf(stderr, "Error: Failed to read component_ptr from %s\n", filename);
        free_graph(graph);
        fclose(fp);
        return NULL;
    }

    // Read components array
    if (fread(graph->components, sizeof(int), components_size, fp) != components_size) {
        fprintf(stderr, "Error: Failed to read components from %s\n", filename);
        free_graph(graph);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return graph;
}

Graph* read_graph(const char *filename) {
    if (detect_file_type(filename) == BINARY_MODE) {
        return read_graph_binary(filename);
    } else {
        return read_graph_text(filename);
    }
}

void write_graph_text(const char *filename, Graph *graph) {
    FILE *out = fopen(filename, "w");
    if (!out) {
        perror("Error writing file");
        return;
    }

    // Section 1
    fprintf(out, "%d\n", graph->max_neighbors);

    // Section 2
    for (int i = 0; i < graph->xadj[graph->nvtxs]; i++) {
        fprintf(out, "%d%c", graph->adjncy[i],
            (i == graph->xadj[graph->nvtxs]-1) ? '\n' : ';');
    }

    // Section 3
    for (int i = 0; i <= graph->nvtxs; i++) {
        fprintf(out, "%d%c", graph->xadj[i],
            (i == graph->nvtxs) ? '\n' : ';');
    }

    // Section 4
    for (int i = 0; i < graph->component_ptr[graph->num_components]; i++) {
        fprintf(out, "%d%c", graph->components[i],
            (i == graph->component_ptr[graph->num_components]-1) ? '\n' : ';');
    }

    // Section 5
    for (int i = 0; i <= graph->num_components; i++) {
        fprintf(out, "%d%c", graph->component_ptr[i],
            (i == graph->num_components) ? '\n' : ';');
    }

    fclose(out);
}

void write_graph_binary(const char *filename, Graph *graph) {
    FILE *out = fopen(filename, "wb");
    if (!out) {
        perror("Error writing binary file");
        return;
    }

    // Write max_neighbors
    fwrite(&graph->max_neighbors, sizeof(int), 1, out);

    // Write xadj size
    int xadj_size = graph->nvtxs + 1;
    fwrite(&xadj_size, sizeof(int), 1, out);

    // Write adjncy size
    int adjncy_size = graph->xadj[graph->nvtxs];
    fwrite(&adjncy_size, sizeof(int), 1, out);

    // Write num_components
    fwrite(&graph->num_components, sizeof(int), 1, out);

    // Write components size
    int components_size = graph->component_ptr[graph->num_components];
    fwrite(&components_size, sizeof(int), 1, out);

    // Write arrays
    fwrite(graph->xadj, sizeof(int), xadj_size, out);
    fwrite(graph->adjncy, sizeof(int), adjncy_size, out);
    fwrite(graph->component_ptr, sizeof(int), graph->num_components + 1, out);
    fwrite(graph->components, sizeof(int), components_size, out);

    fclose(out);
}

void write_graph(const char *filename, Graph *graph, const char *format) {
    if (format && strcmp(format, "binary") == 0) {
        write_graph_binary(filename, graph);
    } else {
        write_graph_text(filename, graph);
    }
}

void free_graph(Graph *graph) {
    if (graph) {
        if (graph->adjncy) free(graph->adjncy);
        if (graph->xadj) free(graph->xadj);
        if (graph->components) free(graph->components);
        if (graph->component_ptr) free(graph->component_ptr);
        free(graph);
    }
}

void print_usage(const char *program_name) {
    printf("Usage: %s <input_file> [num_parts] [error_margine] [format]\n", program_name);
    printf("  input_file: Path to input graph file (.csrrg for text, .bin for binary)\n");
    printf("  num_parts: Number of output parts to generate (default: 1)\n");
    printf("  format: Output format - 'text' or 'binary' (default: same as input)\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Remove old output files
    for (int k = 0; k < 1000; k++) {
        char filename[100];
        sprintf(filename, "part%d.csrrg", k);

        FILE *file = fopen(filename, "r");
        if (!file) continue;
        fclose(file);
        remove(filename);

        sprintf(filename, "part%d.bin", k);
        file = fopen(filename, "r");
        if (!file) continue;
        fclose(file);
        remove(filename);
    }

    // Parse command line arguments
    int num_parts = 1;
    float error_margine=0;
    const char *format = NULL;

    if (argc >= 3) {
        num_parts = atoi(argv[2]);
        if (num_parts < 1) {
            fprintf(stderr, "Error: Number of parts must be ≥ 1\n");
            return 1;
        }
    }

    if (argc >= 4) {
	    error_margine=atof(argv[3]);
	    if(error_margine>100 || error_margine<0) {
		    fprintf(stderr, "Error: Error margine must be  0<=x<=100\n");
		    return 1;
	    }
    }

    if (argc >= 5) {
        format = argv[4];
        if (strcmp(format, "text") != 0 && strcmp(format, "binary") != 0) {
            fprintf(stderr, "Error: Format must be 'text' or 'binary'\n");
            return 1;
        }
    } else {
        // Auto-detect format based on input file extension
        if (strstr(argv[1], ".bin") != NULL) {
            format = "binary";
        } else {
            format = "text";
        }
    }

    // Read input graph
    Graph *graph = read_graph(argv[1]);
    if (!graph) return 1;

    // Wypisanie danych grafu
    print_graph_info(graph, "oryginalny");

    // Przygotowanie do partycjonowania
    float margine = 1.0 + (error_margine)/100; 
    int deleted_edges;

    idx_t *parts = Graph_parts(graph, num_parts, margine, &deleted_edges);
    
    if (parts == NULL) {
        printf("Błąd podczas partycjonowania grafu.\n");
        free_graph(graph);
        return 1;
    }
    
    printf("\nUsuniętych krawędzi: %d\n", deleted_edges);
    printf("Partycje: {");
    for(int i = 0; i < graph->nvtxs; i++) {
        printf("%d", parts[i]);
        if (i < graph->nvtxs - 1) printf(", ");
    }
    printf("}\n");

    // Tworzenie nowych grafów na podstawie partycjonowania
    Graph** New_Graphs = graph_partition(graph, parts, num_parts, margine);
    
    if (New_Graphs == NULL) {
        printf("Błąd podczas tworzenia nowych grafów.\n");
        free(parts);
        free_graph(graph);
        return 1;
    }
    
    // Generate output files
    for (int i = 0; i < num_parts; i++) {
        char filename[100];
	print_graph_info(New_Graphs[i], "podzielony");
        if (strcmp(format, "binary") == 0) {
            sprintf(filename, "part%d.bin", i);
            write_graph(filename, New_Graphs[i], "binary");
        } else {
            sprintf(filename, "part%d.csrrg", i);
            write_graph(filename, New_Graphs[i], "text");

        }

        printf("Generated: %s\n", filename);
	free_graph(New_Graphs[i]);
    }

    free_graph(graph);
    return 0;
}
