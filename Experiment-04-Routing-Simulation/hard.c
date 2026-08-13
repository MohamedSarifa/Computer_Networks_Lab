#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    int target;
    int weight;
    struct Node* next;
} Node;

typedef struct HeapNode {
    int time;
    int node;
} HeapNode;

typedef struct MinHeap {
    HeapNode* data;
    int size;
    int capacity;
} MinHeap;

MinHeap* createMinHeap(int capacity) {
    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));
    heap->data = (HeapNode*)malloc(sizeof(HeapNode) * capacity);
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

void swap(HeapNode* a, HeapNode* b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

void push(MinHeap* heap, int time, int node) {
    if (heap->size == heap->capacity) return;
    
    int i = heap->size++;
    heap->data[i].time = time;
    heap->data[i].node = node;
    
    while (i != 0 && heap->data[(i - 1) / 2].time > heap->data[i].time) {
        swap(&heap->data[i], &heap->data[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

HeapNode pop(MinHeap* heap) {
    HeapNode root = heap->data[0];
    heap->data[0] = heap->data[--heap->size];
    
    int i = 0;
    while (2 * i + 1 < heap->size) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = left;
        
        if (right < heap->size && heap->data[right].time < heap->data[left].time) {
            smallest = right;
        }
        if (heap->data[i].time <= heap->data[smallest].time) {
            break;
        }
        swap(&heap->data[i], &heap->data[smallest]);
        i = smallest;
    }
    return root;
}

int networkDelayTime(int** times, int timesSize, int* timesColSize, int n, int k) {
    Node** graph = (Node**)calloc((n + 1), sizeof(Node*));
    for (int i = 0; i < timesSize; i++) {
        int u = times[i][0];
        int v = times[i][1];
        int w = times[i][2];
        
        Node* newNode = (Node*)malloc(sizeof(Node));
        newNode->target = v;
        newNode->weight = w;
        newNode->next = graph[u];
        graph[u] = newNode;
    }
    
    int* shortest_distances = (int*)malloc((n + 1) * sizeof(int));
    for (int i = 1; i <= n; i++) {
        shortest_distances[i] = -1;
    }
    
    MinHeap* min_heap = createMinHeap(timesSize + 1);
    push(min_heap, 0, k);
    
    int visited_count = 0;
    
    while (min_heap->size > 0) {
        HeapNode curr = pop(min_heap);
        int time = curr.time;
        int node = curr.node;
        
        if (shortest_distances[node] != -1) {
            continue;
        }
        
        shortest_distances[node] = time;
        visited_count++;
        
        Node* neighbor = graph[node];
        while (neighbor != NULL) {
            if (shortest_distances[neighbor->target] == -1) {
                push(min_heap, time + neighbor->weight, neighbor->target);
            }
            neighbor = neighbor->next;
        }
    }
    
    int max_time = 0;
    if (visited_count == n) {
        for (int i = 1; i <= n; i++) {
            if (shortest_distances[i] > max_time) {
                max_time = shortest_distances[i];
            }
        }
    } else {
        max_time = -1;
    }
    
    for (int i = 1; i <= n; i++) {
        Node* curr = graph[i];
        while (curr != NULL) {
            Node* temp = curr;
            curr = curr->next;
            free(temp);
        }
    }
    free(graph);
    free(shortest_distances);
    free(min_heap->data);
    free(min_heap);
    
    return max_time;
}

