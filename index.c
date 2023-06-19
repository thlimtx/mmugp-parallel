#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, wSize, root = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &wSize);

    // get number of files
    int numFile;
    if(rank == 0) {
        printf("Enter the number of files you want to read from:\n");
        scanf("%d", &numFile);
        printf("Number of files: %d\n", numFile);
        
        int size[numFile];
        char *contents[numFile];
        for(int i = 0; i < numFile; i++) {
            char filename[256];
            printf("\nEnter name of file %d:\n", i+1);
            scanf("%s", filename);
            
            FILE *f = fopen(filename, "r");
            fseek(f, 0, SEEK_END);
            size[i] = ftell(f);

            contents[i] = malloc(size[i] +1);
            fread(contents[i], size[i], 1, f);
            fclose(f);
            // contents[i][size[i]] = 0;
            printf("File size %d: %d\n", i + 1, size[i]);
            printf("\nFile content: %s", contents[i]);
        }

    }

    char filenames[numFile];

    MPI_Finalize();
}