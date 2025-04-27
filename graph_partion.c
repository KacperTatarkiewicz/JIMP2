#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <metis.h>
#include "graph_partion.h"

idx_t* Graph_parts(Graph* Origin_Graph, int partions_count, float error_margin, int* deleted_edges)
{
    real_t ubvec = error_margin;
    idx_t nvtxs = Origin_Graph->nvtxs;    // liczba wierzchołków
    idx_t ncon = 1;                       // constraints (METIS wymaga)

    // Tworzenie kopii danych grafu do formatu METIS
    idx_t *xadj = (idx_t*)malloc((nvtxs + 1) * sizeof(idx_t));
    idx_t total_edges = Origin_Graph->xadj[nvtxs];
    idx_t *adjncy = (idx_t*)malloc(total_edges * sizeof(idx_t));
    
    if (!xadj || !adjncy) {
        printf("Błąd alokacji pamięci w Graph_parts\n");
        if (xadj) free(xadj);
        if (adjncy) free(adjncy);
        return NULL;
    }
    
    // Kopiowanie danych
    for (idx_t i = 0; i <= nvtxs; i++) {
        xadj[i] = Origin_Graph->xadj[i];
    }
    
    for (idx_t i = 0; i < total_edges; i++) {
        adjncy[i] = Origin_Graph->adjncy[i];
    }

    idx_t nparts = partions_count;         // liczba partycji
    idx_t objval;                          // funkcja celu (wynik)
    idx_t *part = (idx_t*)malloc(nvtxs * sizeof(idx_t));   // wynikowe partycje

    if (!part) {
        printf("Błąd alokacji pamięci dla tablicy part\n");
        free(xadj);
        free(adjncy);
        return NULL;
    }

    // Inicjalizacja tablicy part
    for (idx_t i = 0; i < nvtxs; i++) {
        part[i] = 0;
    }

    // Wywołanie funkcji 
    int status = METIS_PartGraphRecursive(&nvtxs, &ncon, xadj, adjncy,
                                       NULL, NULL, NULL, &nparts,
                                       NULL, &ubvec, NULL, &objval, part);
    
    // Zapisanie liczby usuniętych krawędzi
    *deleted_edges = objval;
    
    // Zwolnienie tymczasowych kopii
    free(xadj);
    free(adjncy);
    
    if (status == METIS_OK) {
        printf("Partycjonowanie zakończone sukcesem.\n");
    } else {
        printf("Błąd partycjonowania METIS, kod: %d\n", status);
        free(part);
        return NULL;
    }
    
    return part;
}

Graph** graph_partition(Graph* Origin_Graph, idx_t* parts, int partions, float error_margin) {
    int nvtxs = Origin_Graph->nvtxs;
    int *xadj = Origin_Graph->xadj;
    int *adjncy = Origin_Graph->adjncy;
    int *components = Origin_Graph->components;
    int *component_ptr = Origin_Graph->component_ptr;

    // Tworzymy tablicę wynikową
    Graph** New_Graphs = (Graph**)malloc(partions * sizeof(Graph*));
    if (!New_Graphs) {
        printf("Unable to allocate Graph**\n");
        return NULL;
    }

    // Policz liczbę wierzchołków w każdej partycji
    int* vertex_count = (int*)calloc(partions, sizeof(int));
    if (!vertex_count) {
        printf("Unable to allocate vertex_count\n");
        free(New_Graphs);
        return NULL;
    }
    
    for (int i = 0; i < nvtxs; i++) {
        if (parts[i] < 0 || parts[i] >= partions) {
            printf("Invalid part number for vertex %d: %d\n", i, parts[i]);
            free(New_Graphs);
            free(vertex_count);
            return NULL;
        }
        vertex_count[parts[i]]++;
    }

    // Mapowanie oryginalnych indeksów wierzchołków na nowe indeksy w partycjach
    int** vertex_map = (int**)malloc(partions * sizeof(int*));
    int** reverse_map = (int**)malloc(partions * sizeof(int*));
    
    if (!vertex_map || !reverse_map) {
        printf("Unable to allocate mapping arrays\n");
        free(New_Graphs);
        free(vertex_count);
        if (vertex_map) free(vertex_map);
        if (reverse_map) free(reverse_map);
        return NULL;
    }
    
    for (int i = 0; i < partions; i++) {
        vertex_map[i] = (int*)malloc(nvtxs * sizeof(int));
        reverse_map[i] = (int*)malloc(vertex_count[i] * sizeof(int));
        
        if (!vertex_map[i] || !reverse_map[i]) {
            printf("Unable to allocate mapping arrays for partition %d\n", i);
            for (int j = 0; j <= i; j++) {
                if (vertex_map[j]) free(vertex_map[j]);
                if (reverse_map[j]) free(reverse_map[j]);
            }
            free(vertex_map);
            free(reverse_map);
            free(New_Graphs);
            free(vertex_count);
            return NULL;
        }
        
        int idx = 0;
        for (int v = 0; v < nvtxs; v++) {
            if (parts[v] == i) {
                vertex_map[i][v] = idx;
                reverse_map[i][idx] = v;
                idx++;
            } else {
                vertex_map[i][v] = -1;  // -1 oznacza, że wierzchołek nie należy do tej partycji
            }
        }
    }

    // Policzenie liczby krawędzi wewnętrznych dla każdej partycji
    int* edge_count = (int*)calloc(partions, sizeof(int));
    if (!edge_count) {
        printf("Unable to allocate edge_count\n");
        free(New_Graphs);
        free(vertex_count);
        for (int i = 0; i < partions; i++) {
            free(vertex_map[i]);
            free(reverse_map[i]);
        }
        free(vertex_map);
        free(reverse_map);
        return NULL;
    }

    for (int v = 0; v < nvtxs; v++) {
        int part_v = parts[v];
        for (int e = xadj[v]; e < xadj[v + 1]; e++) {
            int u = adjncy[e];
            if (u < nvtxs && parts[u] == part_v) {  // Dodane sprawdzenie u < nvtxs
                edge_count[part_v]++;
            }
        }
    }

    // Teraz alokujemy strukturę Graph dla każdej partycji
    for (int i = 0; i < partions; i++) {
        New_Graphs[i] = (Graph*)malloc(sizeof(Graph));
        if (!New_Graphs[i]) {
            printf("Unable to allocate Graph %d\n", i);
            for (int j = 0; j < i; j++) {
                free(New_Graphs[j]->adjncy);
                free(New_Graphs[j]->xadj);
                free(New_Graphs[j]->components);
                free(New_Graphs[j]->component_ptr);
                free(New_Graphs[j]);
            }
            free(New_Graphs);
            free(vertex_count);
            free(edge_count);
            for (int j = 0; j < partions; j++) {
                free(vertex_map[j]);
                free(reverse_map[j]);
            }
            free(vertex_map);
            free(reverse_map);
            return NULL;
        }

        New_Graphs[i]->nvtxs = vertex_count[i];
        New_Graphs[i]->num_components = 0;  // Będziemy obliczać później
        
        // Wyznaczamy max_neighbors dla tej partycji
        int max_n = 0;
        for (int j = 0; j < vertex_count[i]; j++) {
            int orig_v = reverse_map[i][j];
            int degree = xadj[orig_v + 1] - xadj[orig_v];
            if (degree > max_n) max_n = degree;
        }
        New_Graphs[i]->max_neighbors = max_n;

        New_Graphs[i]->adjncy = (int*)malloc(edge_count[i] * sizeof(int));
        New_Graphs[i]->xadj = (int*)malloc((vertex_count[i] + 1) * sizeof(int));
        New_Graphs[i]->components = (int*)malloc(vertex_count[i] * sizeof(int));
        New_Graphs[i]->component_ptr = (int*)malloc((vertex_count[i] + 1) * sizeof(int));

        if (!New_Graphs[i]->adjncy || !New_Graphs[i]->xadj || 
            !New_Graphs[i]->components || !New_Graphs[i]->component_ptr) {
            printf("Unable to allocate arrays inside Graph partition %d.\n", i);
            for (int j = 0; j <= i; j++) {
                free(New_Graphs[j]->adjncy);
                free(New_Graphs[j]->xadj);
                free(New_Graphs[j]->components);
                free(New_Graphs[j]->component_ptr);
                free(New_Graphs[j]);
            }
            free(New_Graphs);
            free(vertex_count);
            free(edge_count);
            for (int j = 0; j < partions; j++) {
                free(vertex_map[j]);
                free(reverse_map[j]);
            }
            free(vertex_map);
            free(reverse_map);
            return NULL;
        }
    }

    // Wypełnianie grafów
    int* current_edge = (int*)calloc(partions, sizeof(int));
    if (!current_edge) {
        printf("Unable to allocate current_edge\n");
        for (int i = 0; i < partions; i++) {
            free(New_Graphs[i]->adjncy);
            free(New_Graphs[i]->xadj);
            free(New_Graphs[i]->components);
            free(New_Graphs[i]->component_ptr);
            free(New_Graphs[i]);
        }
        free(New_Graphs);
        free(vertex_count);
        free(edge_count);
        for (int j = 0; j < partions; j++) {
            free(vertex_map[j]);
            free(reverse_map[j]);
        }
        free(vertex_map);
        free(reverse_map);
        return NULL;
    }

    // Ustaw początkowe wartości xadj
    for (int p = 0; p < partions; p++) {
        New_Graphs[p]->xadj[0] = 0;
    }

    // Wypełnianie adjncy i xadj
    for (int p = 0; p < partions; p++) {
        for (int local_v = 0; local_v < vertex_count[p]; local_v++) {
            int orig_v = reverse_map[p][local_v];
            
            // Dodawanie sąsiadów do tablicy adjncy i przeliczanie indeksów
            for (int e = xadj[orig_v]; e < xadj[orig_v + 1]; e++) {
                int orig_u = adjncy[e];
                if (orig_u < nvtxs && parts[orig_u] == p) {
                    // Przeliczamy indeks sąsiada na lokalny indeks w partycji
                    int local_u = vertex_map[p][orig_u];
                    New_Graphs[p]->adjncy[current_edge[p]++] = local_u;
                }
            }
            
            // Ustawianie xadj dla następnego wierzchołka
            New_Graphs[p]->xadj[local_v + 1] = current_edge[p];
            
            // Kopiowanie danych o komponentach
            New_Graphs[p]->components[local_v] = components[orig_v];
        }
    }

    // Aktualizacja struktur komponentów dla każdej partycji
    for (int p = 0; p < partions; p++) {
        // Tworzymy kopię tablicy components
        int* tmp_components = (int*)malloc(vertex_count[p] * sizeof(int));
        if (!tmp_components) {
            printf("Unable to allocate temporary components array\n");
            // Tutaj należałoby zwolnić wszystkie zaalokowane zasoby
            // ale dla uproszczenia kontynuujemy z oryginalną strukturą komponentów
        } else {
            // Kopiujemy dane komponentów
            for (int i = 0; i < vertex_count[p]; i++) {
                tmp_components[i] = New_Graphs[p]->components[i];
            }
            
            // Sortujemy komponenty (prosta implementacja)
            // W rzeczywistej implementacji należałoby użyć efektywniejszej metody
            int num_unique_components = 0;
            int last_component = -1;
            
            for (int i = 0; i < vertex_count[p]; i++) {
                int comp_found = 0;
                for (int j = 0; j < num_unique_components; j++) {
                    if (New_Graphs[p]->components[i] == tmp_components[j]) {
                        comp_found = 1;
                        break;
                    }
                }
                
                if (!comp_found) {
                    tmp_components[num_unique_components++] = New_Graphs[p]->components[i];
                }
            }
            
            // Sortujemy unikalne komponenty
            for (int i = 0; i < num_unique_components; i++) {
                for (int j = i + 1; j < num_unique_components; j++) {
                    if (tmp_components[i] > tmp_components[j]) {
                        int temp = tmp_components[i];
                        tmp_components[i] = tmp_components[j];
                        tmp_components[j] = temp;
                    }
                }
            }
            
            // Aktualizujemy component_ptr
            New_Graphs[p]->num_components = num_unique_components;
            int current_pos = 0;
            
            for (int i = 0; i < num_unique_components; i++) {
                New_Graphs[p]->component_ptr[i] = current_pos;
                for (int j = 0; j < vertex_count[p]; j++) {
                    if (New_Graphs[p]->components[j] == tmp_components[i]) {
                        current_pos++;
                    }
                }
            }
            
            New_Graphs[p]->component_ptr[num_unique_components] = vertex_count[p];
            free(tmp_components);
        }
    }

    // Sprzątanie tymczasowych tablic
    free(vertex_count);
    free(edge_count);
    free(current_edge);
    for (int i = 0; i < partions; i++) {
        free(vertex_map[i]);
        free(reverse_map[i]);
    }
    free(vertex_map);
    free(reverse_map);

    return New_Graphs;
}

void print_graph_info(Graph* graph, const char* name) {
    printf("\n=== Graf %s ===\n", name);
    printf("Liczba wierzchołków: %d\n", graph->nvtxs);
    printf("Liczba komponentów: %d\n", graph->num_components);
    printf("Max sąsiadów: %d\n", graph->max_neighbors);
    
    printf("xadj: ");
    for (int i = 0; i <= graph->nvtxs; i++) {
        printf("%d ", graph->xadj[i]);
    }
    printf("\n");
    
    printf("adjncy: ");
    for (int i = 0; i < graph->xadj[graph->nvtxs]; i++) {
        printf("%d ", graph->adjncy[i]);
    }
    printf("\n");
    
    printf("components: ");
    for (int i = 0; i < graph->nvtxs; i++) {
        printf("%d ", graph->components[i]);
    }
    printf("\n");
    
    printf("component_ptr: ");
    for (int i = 0; i <= graph->num_components; i++) {
        printf("%d ", graph->component_ptr[i]);
    }
    printf("\n");
}

