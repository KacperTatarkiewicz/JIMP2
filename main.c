#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <metis.h>

#define MAX_LINE 1000000

typedef struct {
    int max_neighbors;
    int *adjncy;
    int *xadj;
    int nvtxs;
    int *components;
    int *component_ptr;
    int num_components;
} SubGraph;

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

void find_components(SubGraph *sg) {
    int *visited = calloc(sg->nvtxs, sizeof(int));
    sg->components = malloc(sg->nvtxs * sizeof(int));
    sg->component_ptr = malloc((sg->nvtxs + 1) * sizeof(int));
    sg->num_components = 0;
    int comp_index = 0;

    for (int i = 0; i < sg->nvtxs; i++) {
        if (!visited[i]) {
            sg->component_ptr[sg->num_components++] = comp_index;
            // BFS
            int *queue = malloc(sg->nvtxs * sizeof(int));
            int front = 0, rear = 0;
            queue[rear++] = i;
            visited[i] = 1;

            while (front < rear) {
                int u = queue[front++];
                sg->components[comp_index++] = u;
                for (int j = sg->xadj[u]; j < sg->xadj[u+1]; j++) {
                    int v = sg->adjncy[j];
                    if (!visited[v]) {
                        visited[v] = 1;
                        queue[rear++] = v;
                    }
                }
            }
            free(queue);
        }
    }
    sg->component_ptr[sg->num_components] = comp_index;
    free(visited);
}

void write_subgraph(FILE *out, SubGraph *sg) {
    // Sekcja 1
    fprintf(out, "%d\n", sg->max_neighbors);

    // Sekcja 2
    for (int i = 0; i < sg->xadj[sg->nvtxs]; i++) {
        fprintf(out, "%d%c", sg->adjncy[i], (i == sg->xadj[sg->nvtxs]-1) ? '\n' : ';');
    }

    // Sekcja 3
    for (int i = 0; i <= sg->nvtxs; i++) {
        fprintf(out, "%d%c", sg->xadj[i], (i == sg->nvtxs) ? '\n' : ';');
    }

    // Sekcja 4 i 5
    find_components(sg);
    for (int i = 0; i < sg->component_ptr[sg->num_components]; i++) {
        fprintf(out, "%d%c", sg->components[i], (i == sg->component_ptr[sg->num_components]-1) ? '\n' : ';');
    }
    for (int i = 0; i <= sg->num_components; i++) {
        fprintf(out, "%d%c", sg->component_ptr[i], (i == sg->num_components) ? '\n' : ';');
    }
}

int main(int argc, char **argv) {
    if (argc < 3) { // Zmiana z 3 na 4
        printf("Użycie: %s <plik_we> <liczba_części> <procent_nierównowagi>\n", argv[0]);
        return 1;
    }
    float percentage;
    if (argc>3 && argv[3]>0) percentage = atof(argv[3]);
    else percentage = 10.0;

    float ubvec_value = 1.0 + (percentage / 100.0);

    // Usuń stare pliki przed rozpoczęciem
    int nParts = atoi(argv[2]);

    for (int k = 0; k < 1000; k++) {
        char filename[100];
        sprintf(filename, "part%d.csrrg", k);

        FILE *file = fopen(filename, "r");
        if (!file) {
            // Plik nie istnieje, wyjdź z pętli
            break;
        }
        fclose(file); // Zamknij plik, jeśli istnieje

        remove(filename); // Usuń plik
    }
    // Wczytaj oryginalny graf
    FILE *fp = fopen(argv[1], "r");


    // Sekcja 1 (pominięta)
    char line[MAX_LINE];
    fgets(line, MAX_LINE, fp);

    // Sekcja 2 (adjncy)
    int adjncy_size = 0;
    fgets(line, MAX_LINE, fp);
    int *adjncy = parse_section(line, &adjncy_size);

    // Sekcja 3 (xadj)
    int xadj_size = 0;
    fgets(line, MAX_LINE, fp);
    int *xadj = parse_section(line, &xadj_size);
    int nvtxs = xadj_size - 1;

    // Partycjonowanie METIS
    int *part = malloc(nvtxs * sizeof(int));
    int objval;
    int ncon = 1;
    float ubvec[] = {ubvec_value};
    METIS_PartGraphKway(
        &nvtxs, &ncon, xadj, adjncy,
        NULL, NULL, NULL, &nParts,
        NULL,    // tpwgts (automatyczna dystrybucja)
        ubvec,   // ubvec we właściwym miejscu
        NULL, &objval, part
    );

    // Przetwarzaj każdą część
    for (int k = 0; k < nParts; k++) {
        SubGraph sg = {0};
        int *map = malloc(nvtxs * sizeof(int));
        memset(map, -1, nvtxs * sizeof(int));

        // Zbierz węzły w części
        int cnt = 0;
        for (int i = 0; i < nvtxs; i++) {
            if (part[i] == k) map[i] = cnt++;
        }

        // Buduj podgraf
        sg.nvtxs = cnt;
        sg.xadj = calloc(cnt + 1, sizeof(int));

        // Oblicz xadj
        for (int i = 0; i < nvtxs; i++) {
            if (map[i] == -1) continue;
            int edges = 0;
            for (int j = xadj[i]; j < xadj[i+1]; j++) {
                if (map[adjncy[j]] != -1) edges++;
            }
            sg.xadj[map[i]+1] = sg.xadj[map[i]] + edges;
        }

        // Oblicz adjncy
        sg.adjncy = malloc(sg.xadj[cnt] * sizeof(int));
        int pos = 0;
        for (int i = 0; i < nvtxs; i++) {
            if (map[i] == -1) continue;
            for (int j = xadj[i]; j < xadj[i+1]; j++) {
                if (map[adjncy[j]] != -1) {
                    sg.adjncy[pos++] = map[adjncy[j]];
                }
            }
        }

        // Oblicz maksymalną liczbę sąsiadów
        sg.max_neighbors = 0;
        for (int i = 0; i < cnt; i++) {
            int size = sg.xadj[i+1] - sg.xadj[i];
            if (size > sg.max_neighbors) sg.max_neighbors = size;
        }

        // Zapisz do pliku
        char filename[100];
        sprintf(filename, "part%d.csrrg", k);
        FILE *out = fopen(filename, "w");
        write_subgraph(out, &sg);
        fclose(out);

        free(map);
        free(sg.xadj);
        free(sg.adjncy);
    }

    free(adjncy);
    free(xadj);
    free(part);
    return 0;
}
