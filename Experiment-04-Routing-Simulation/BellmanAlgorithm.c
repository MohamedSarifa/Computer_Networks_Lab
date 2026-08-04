#include <stdio.h>

#define MAX_ROUTERS 20
#define INF 9999

struct Router {
    int distance[MAX_ROUTERS];
    int nextHop[MAX_ROUTERS];
};

int main() {
    int n = 0;
    int costMatrix[MAX_ROUTERS][MAX_ROUTERS];
    struct Router routingTable[MAX_ROUTERS];

    printf("==========================================\n");
    printf("        DISTANCE VECTOR ROUTING           \n");
    printf("==========================================\n");

    printf("Enter the number of routers: ");
    scanf("%d", &n);

    if (n <= 0 || n > MAX_ROUTERS) {
        printf("⚠️ Error: Invalid number of routers.\n");
        return 1;
    }

    printf("\nEnter the cost matrix (Use %d for Infinity/No Direct Link):\n", INF);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            scanf("%d", &costMatrix[i][j]);

            if (i == j) {
                costMatrix[i][j] = 0;
            }
        }
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            routingTable[i].distance[j] = costMatrix[i][j];

            if (costMatrix[i][j] != INF && i != j) {
                routingTable[i].nextHop[j] = j;
            } else {
                routingTable[i].nextHop[j] = i;
            }
        }
    }

    int changesOccurred = 0;
    int iterationCount = 0;

    for(int iter=0;iter<n-1;iter++)
    {
        changesOccurred = 0;
        iterationCount++;

        for (int i = 0; i < n; i++) {

            for (int j = 0; j < n; j++) {

                for (int k = 0; k < n; k++) {
                    if (costMatrix[i][k] != INF && routingTable[k].distance[j] != INF) {
                        int newDistance = costMatrix[i][k] + routingTable[k].distance[j];
                        if(newDistance < 0)
                        {
                                printf("\nNegative Cycle Edge Can't Compute\n");
                                return 0;
                        }
                        if (newDistance < routingTable[i].distance[j]) {
                            routingTable[i].distance[j] = newDistance;
                            routingTable[i].nextHop[j] = routingTable[i].nextHop[k];

                            changesOccurred = 1;
                        }
                    }
                }
            }
        }
    }

    printf("\n==========================================\n");
    printf("      FINAL ROUTING TABLES (%d iterations) \n", iterationCount);
    printf("==========================================\n");

    for (int i = 0; i < n; i++) {
        printf("\n--- Routing Table for Router %d ---\n", i + 1);
        printf("Destination | Shortest Distance | Next Hop\n");
        printf("------------|-------------------|---------\n");

        for (int j = 0; j < n; j++) {
            if (routingTable[i].distance[j] == INF) {
                printf("     %d      |        INF        |    -\n", j + 1);
            } else {
                printf("     %d      |        %2d         |    %d\n",
                       j + 1,
                       routingTable[i].distance[j],
                       routingTable[i].nextHop[j] + 1);
            }
        }
    }

    return 0;
}
