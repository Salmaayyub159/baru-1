#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cilk/cilkscale.h>

#define MAX_SIZE 100
#define FLAG_MAX 9999
#define PREVIOUS_FILES 100

int main(int argc, char const **argv) {

    int N = -1; 
    unsigned int K = 0; 
    unsigned int D = 0; 
    srand(time(NULL)); 
    unsigned int randVar = 0;
    unsigned int iteration = 0;
    int flagEnd = 0;
    clock_t start, end;

    register int i;
    register int j;
    register int d;

    char filename[MAX_SIZE];
    unsigned int choise = 0;

    FILE *Dataset; 

    printf("\n Input Dataset filename : ");
    scanf("%s", filename);
    printf("\n");

    if (access(filename, F_OK) == -1) {
        printf(" There is something wrong with the File\n");
        while (choise == 0) {
            printf("\n Input New File(Type 0 to Exit): ");
            scanf("%s", filename);
            printf("\n");
            if (strcmp(filename, "0") == 0) {
                printf("\n Program has been terminated\n\n");
                return -1;
            } else if (access(filename, F_OK) == -1) {
                choise = 0;
            } else {
                choise = 1;
            }
        }
    }

    Dataset = fopen(filename, "r");

    int flag = 0;
    int flagPrev = 0;
    int counter = 0;
    char c;

    while (c != '\n') {
        fscanf(Dataset, "%c", &c);
        if ((c >= '0' && c <= '9') || (c == '-' || c == '+') || (c == '.')) {
            flag = 1;
            counter++;
        } else {
            flag = 0;
            counter = 0;
        }
        if (counter == 0 && flagPrev != flag) {
            ++D;
        }
        flagPrev = flag;
    }
    rewind(Dataset);

    while (!feof(Dataset)) {
        fscanf(Dataset, "%c", &c);
        if (c == '\n') ++N;
    }
    rewind(Dataset);

    printf("\n Size of Data : %d\n", N);
    printf("\n Number of features : %d\n", D);

    printf("\n Give Clusters(k > 0 required) : ");
    scanf("%d", &K);
    printf("\n");

    if (K <= 0) {
        printf("\n Can't be executed with K = %d!(k > 0 required)\n", K);
        printf("\n Program has been terminated\n\n");
        return -2;
    }

    float *DataArray = (float *)calloc(N * D, sizeof(float));
    float *Centroids = (float *)calloc(K * D, sizeof(float));
    float *FlagCentroids = (float *)calloc(K * D, sizeof(float));
    int *Counter = (int *)calloc(K, sizeof(int));
    float *ClusterTotalSum = (float *)calloc(K * D, sizeof(float));
    float *Distance = (float *)calloc(N * K, sizeof(float));
    float *Min = (float *)calloc(N, sizeof(float));
    int *Location = (int *)calloc(N, sizeof(int));

    for (i = 0; i < N; ++i) {
        for (d = 0; d < D; ++d) {
            fscanf(Dataset, "%f,", &DataArray[i * D + d]);
        }
    }
    fclose(Dataset);

    for (j = 0; j < K; ++j) {
        randVar = rand() % N;
        for (d = 0; d < D; ++d) {
            Centroids[j * D + d] = DataArray[randVar * D + d];
            FlagCentroids[j * D + d] = Centroids[j * D + d];
        }
    }

    start = clock();

    do {
        if (iteration > 0) {
            cilk_for (j = 0; j < K; ++j) {
                Counter[j] = 0;
                for (d = 0; d < D; ++d) {
                    ClusterTotalSum[j * D + d] = 0;
                }
            }
        }

        cilk_for (i = 0; i < N; ++i) {
            Min[i] = FLAG_MAX;
            for (j = 0; j < K; ++j) {
                Distance[i * K + j] = 0;
                for (d = 0; d < D; ++d) {
                    Distance[i * K + j] += ((DataArray[i * D + d] - Centroids[j * D + d]) * 
                                            (DataArray[i * D + d] - Centroids[j * D + d]));
                }
                Distance[i * K + j] = sqrt(Distance[i * K + j]);

                if (Distance[i * K + j] < Min[i]) {
                    Min[i] = Distance[i * K + j];
                    Location[i] = j;
                }
            }

            for (j = 0; j < K; ++j) {
                if (Location[i] == j) {
                    for (d = 0; d < D; ++d) {
                        ClusterTotalSum[j * D + d] += DataArray[i * D + d];
                    }
                    ++Counter[j];
                }
            }
        }

        cilk_for (j = 0; j < K; ++j) {
            for (d = 0; d < D; ++d) {
                Centroids[j * D + d] = ClusterTotalSum[j * D + d] / Counter[j];
            }
        }

        flagEnd = -1;
        cilk_for (j = 0; j < K; ++j) {
            for (d = 0; d < D; ++d) {
                if (FlagCentroids[j * D + d] != Centroids[j * D + d]) {
                    flagEnd = 0;
                    break;
                }
            }
        }

        cilk_for (j = 0; j < K; ++j) {
            for (d = 0; d < D; ++d) {
                FlagCentroids[j * D + d] = Centroids[j * D + d];
            }
        }

        ++iteration;

    } while (flagEnd != -1);

    end = clock();

    printf("\n Iterations : %d\n", iteration);
    double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("\n Time of Algorithm Execution: %lf \n\n", total_time);

    FILE **OutputArray;
    OutputArray = calloc(K, sizeof(FILE *));
    char fileName[MAX_SIZE];

    for (j = 0; j < K; ++j) {
        sprintf(fileName, "Cluster_%d.txt", j);
        OutputArray[j] = fopen(fileName, "w");
    }

    cilk_for (j = 0; j < K; ++j) {
        for (i = 0; i < N; ++i) {
            if (Location[i] == j) {
                for (d = 0; d < D; ++d) {
                    fprintf(OutputArray[j], "%f ", DataArray[i * D + d]);
                }
                fprintf(OutputArray[j], "\n");
            }
        }
    }

    cilk_for (j = 0; j < K; ++j) {
        fclose(OutputArray[j]);
    }

    cilk_for (j = PREVIOUS_FILES; j >= K; --j) {
        sprintf(fileName, "Cluster_%d.txt", j);
        remove(fileName);
    }

    free(DataArray);
    free(Centroids);
    free(FlagCentroids);
    free(Counter);
    free(ClusterTotalSum);
    free(Distance);
    free(Min);
    free(Location);
    free(OutputArray);

    return 0;
}
