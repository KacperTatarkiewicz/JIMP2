#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1000000

typedef struct {
    int max_neighbors;
    int *adjncy;
    int *xadj;
    int nvtxs;
    int *components;
    int *component_ptr;
    int num_components;
} Graph;

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

Graph* read_graph(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Błąd: nie można otworzyć pliku %s\n", filename);
        return NULL;
    }

    Graph *graph = malloc(sizeof(Graph));
    char line[MAX_LINE];

    // Sekcja 1: max_neighbors
    fgets(line, MAX_LINE, fp);
    graph->max_neighbors = atoi(line);

    // Sekcja 2: adjncy
    int adjncy_size = 0;
    fgets(line, MAX_LINE, fp);
    graph->adjncy = parse_section(line, &adjncy_size);

    // Sekcja 3: xadj
    int xadj_size = 0;
    fgets(line, MAX_LINE, fp);
    graph->xadj = parse_section(line, &xadj_size);
    graph->nvtxs = xadj_size - 1;

    // Sekcja 4: components
    int components_size = 0;
    fgets(line, MAX_LINE, fp);
    graph->components = parse_section(line, &components_size);

    // Sekcja 5: component_ptr
    int component_ptr_size = 0;
    fgets(line, MAX_LINE, fp);
    graph->component_ptr = parse_section(line, &component_ptr_size);
    graph->num_components = component_ptr_size - 1;

    fclose(fp);
    return graph;
}

void write_output(const char *filename, Graph *graph) {
    FILE *out = fopen(filename, "w");
    if (!out) {
        perror("Błąd zapisu pliku");
        return;
    }

    // Sekcja 1
    fprintf(out, "%d\n", graph->max_neighbors);

    // Sekcja 2
    for (int i = 0; i < graph->xadj[graph->nvtxs]; i++) {
        fprintf(out, "%d%c", graph->adjncy[i], 
            (i == graph->xadj[graph->nvtxs]-1) ? '\n' : ';');
    }

    // Sekcja 3
    for (int i = 0; i <= graph->nvtxs; i++) {
        fprintf(out, "%d%c", graph->xadj[i], 
            (i == graph->nvtxs) ? '\n' : ';');
    }

    // Sekcja 4
    for (int i = 0; i < graph->component_ptr[graph->num_components]; i++) {
        fprintf(out, "%d%c", graph->components[i], 
            (i == graph->component_ptr[graph->num_components]-1) ? '\n' : ';');
    }

    // Sekcja 5
    for (int i = 0; i <= graph->num_components; i++) {
        fprintf(out, "%d%c", graph->component_ptr[i], 
            (i == graph->num_components) ? '\n' : ';');
    }

    fclose(out);
}

void free_graph(Graph *graph) {
    free(graph->adjncy);
    free(graph->xadj);
    free(graph->components);
    free(graph->component_ptr);
    free(graph);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Użycie: %s <wejście.csrrg> [liczba_części]\n", argv[0]);
        return 1;
    }

    // Usuwanie starych plików
    for (int k = 0; k < 1000; k++) {
        char filename[100];
        sprintf(filename, "part%d.csrrg", k);

        FILE *file = fopen(filename, "r");
        if (!file) continue;
        fclose(file);
        remove(filename);
    }

    int num_parts = 1;
    if (argc >= 3) {
        num_parts = atoi(argv[2]);
        if (num_parts < 1) {
            fprintf(stderr, "Błąd: liczba części musi być ≥ 1\n");
            return 1;
        }
    }

    Graph *graph = read_graph(argv[1]);
    if (!graph) return 1;

    for (int i = 0; i < num_parts; i++) {
        char filename[100];
        sprintf(filename, "part%d.csrrg", i);
        write_output(filename, graph);
    }

    free_graph(graph);
    return 0;
}
