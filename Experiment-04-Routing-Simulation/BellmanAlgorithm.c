#include <stdio.h>
#define MAX_ROUTERS 20
#define INF 9999

typedef struct {
    int src, dest, weight;
} Edge;

void printPath(int parent[], int j) {
    if (parent[j] == -1) {
        printf("%d", j + 1);
        return;
    }
    printPath(parent, parent[j]);
    printf(" -> %d", j + 1);
}

int main() {
    int n = 0;
    int costMatrix[MAX_ROUTERS][MAX_ROUTERS];
    int dist[MAX_ROUTERS];
    int parent[MAX_ROUTERS];
    int source = 0;
    Edge edges[MAX_ROUTERS * MAX_ROUTERS];
    int edgeCount = 0;

    printf("==========================================\n");
    printf("        DISTANCE VECTOR / BELLMAN-FORD     \n");
    printf("==========================================\n");

    printf("Enter the number of routers (nodes): ");
    scanf("%d", &n);

    if (n <= 0 || n > MAX_ROUTERS) {
        printf(" Error: Invalid number of routers.\n");
        return 1;
    }

    printf("\nEnter the cost matrix (use %d for Infinity/No link):\n", INF);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            scanf("%d", &costMatrix[i][j]);

            if (i == j) {
                costMatrix[i][j] = 0;
            }
            else if (costMatrix[i][j] != INF) {
                edges[edgeCount].src = i;
                edges[edgeCount].dest = j;
                edges[edgeCount].weight = costMatrix[i][j];
                edgeCount++;
            }
        }
    }
    printf("\nEnter the source router (1 to %d): ", n);
    scanf("%d", &source);

    if (source < 1 || source > n) {
        printf(" Error: Invalid source router selection.\n");
        return 1;
    }

    int srcIdx = source - 1;
    for (int i = 0; i < n; i++) {
        dist[i] = INF;
        parent[i] = -1;
    }
    dist[srcIdx] = 0;
    for (int i = 1; i <= n - 1; i++) {
        for (int j = 0; j < edgeCount; j++) {
            int u = edges[j].src;
            int v = edges[j].dest;
            int weight = edges[j].weight;

            if (dist[u] != INF && dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                parent[v] = u;
            }
        }
    }

    int hasNegativeCycle = 0;
    for (int j = 0; j < edgeCount; j++) {
        int u = edges[j].src;
        int v = edges[j].dest;
        int weight = edges[j].weight;

        if (dist[u] != INF && dist[u] + weight < dist[v]) {
            hasNegativeCycle = 1;
            break;
        }
    }

    if (hasNegativeCycle) {
        printf(" Error: Graph contains a negative-weight cycle! Shortest paths cannot be calculated.\n");
        return 1;
    }

    printf("\n==========================================\n");
    printf("       SHORTEST PATHS FROM ROUTER %d        \n", source);
    printf("==========================================\n");
    printf("Destination | Shortest Cost | Path\n");
    printf("------------|---------------|-------------------\n");
    for (int i = 0; i < n; i++) {
        if (dist[i] == INF) {
            printf("     %d      |      INF      | No Path\n", i + 1);
        } else {
            printf("     %d      |      %2d       | ", i + 1, dist[i]);
            printPath(parent, i);
            printf("\n");
        }
    }

    return 0;
}
