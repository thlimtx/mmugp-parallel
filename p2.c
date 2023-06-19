#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <mpi.h>
#include <time.h>


struct Frequency
{
    char word[100];
    int count;
};


int alphabetical(const void *a, const void *b)
{
    const struct Frequency *structA = (const struct Frequency *)a;
    const struct Frequency *structB = (const struct Frequency *)b;
    return strcmp(structA->word, structB->word);
}


int numerical(const void *a, const void *b)
{
    const struct Frequency *structA = (const struct Frequency *)a;
    const struct Frequency *structB = (const struct Frequency *)b;
    return structB->count - structA->count;
}


int main(int argc, char *argv[])
{
    int file_count = 0, minlen, maxlen, file_size, size = 0, local_start, local_end;
    char output_order;
    struct Frequency *array = (struct Frequency *)malloc(sizeof(struct Frequency) * 1);
    char **file_name;
    FILE *infile;
    FILE *outfile;


    int np, rank;
    MPI_Status status;


    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    if (!rank)
    {
        printf("Enter the number of text files: ");
        fflush(stdout);
        scanf("%d", &file_count);


        file_name = (char **)malloc(sizeof(char *) * file_count);
        for (int i = 0; i < file_count; i++)
        {
            file_name[i] = (char *)malloc(sizeof(char) * 100);
            printf("Enter the path of text file %d: ", i + 1);
            fflush(stdout);
            scanf("%s", file_name[i]);
        }
        printf("Enter the minimum length of words to consider: ");
        fflush(stdout);
        scanf("%d", &minlen);


        printf("Enter the maximum length of words to consider: ");
        fflush(stdout);
        scanf("%d", &maxlen);


        printf("Enter 'a' for alphabetical order or 'n' for number of words order: ");
        fflush(stdout);
        scanf(" %c", &output_order);
    }


    MPI_Bcast(&file_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0)
        file_name = (char **)malloc(sizeof(char *) * file_count);
    for (int i = 0; i < file_count; i++)
    {
        if (rank != 0)
            file_name[i] = (char *)malloc(sizeof(char) * 100);
        int temp = strlen(file_name[i]) + 1;
        MPI_Bcast(file_name[i], temp, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    printf("%lf \n", MPI_Wtime());
    // Start
    int count = 0;
    int array_size = 0;
    while (count < file_count)
    {
        int max_line = 0, local_size = 0;
        infile = fopen(file_name[count], "r");
        if (infile == NULL)
        {
            perror("Opening file");
            exit(1);
        }


        if (!rank)
        {
            char c;
            for (c = getc(infile); c != EOF; c = getc(infile))
                if (c == '\n')
                    max_line++;
        }


        MPI_Bcast(&max_line, 1, MPI_INT, 0, MPI_COMM_WORLD);


        fseek(infile, 0, SEEK_END);
        file_size = ftell(infile);
        fseek(infile, 0, SEEK_SET);


        int chunk_size = max_line / np;
        local_start = rank * chunk_size;


        if (rank == np - 1)
            local_end = max_line - 1;
        else
            local_end = (rank + 1) * chunk_size - 1;


        struct Frequency *local_array = (struct Frequency *)malloc(sizeof(struct Frequency) * 1);


        char line[1000];
        const char *s = " ";
        int line_num = local_start;
        while ((line_num <= local_end && fgets(line, sizeof(line), infile) != NULL))
        {
            for (int i = 0; line[i] != '\0'; i++)
            {
                if ((line[i] >= 65 && line[i] <= 90) || (line[i] >= 97 && line[i] <= 122))
                    line[i] = tolower(line[i]);
                else
                    line[i] = ' ';
            }


            char *word = strtok(line, s);
            while (word != NULL)
            {
                int found = 0;
                for (int i = 0; i < local_size; i++)
                {
                    if (strcmp(local_array[i].word, word) == 0)
                    {
                        found = 1;
                        local_array[i].count++;
                        break;
                    }
                }
                if (found == 0)
                {
                    local_array = realloc(local_array, sizeof(struct Frequency) * (local_size + 1));
                    strcpy(local_array[local_size].word, word);
                    local_array[local_size].count++;
                    local_size++;
                }
                word = strtok(NULL, s);
            }


            line_num++;
        }
        printf("%lf \n", MPI_Wtime());
        if (!rank)
        {
            for (int i = 1; i < np; i++)
            {
                int array_size = 0;
                MPI_Recv(&array_size, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
                for (int j = 0; j < array_size; j++)
                {
                    int temp, count, found = 0;


                    MPI_Recv(&temp, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
                    char *temp_word = malloc(sizeof(char) * temp);
                    MPI_Recv(temp_word, temp, MPI_CHAR, i, 1, MPI_COMM_WORLD, &status);
                    MPI_Recv(&count, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status);


                    for (int k = 0; k < local_size; k++)
                    {
                        if (strcmp(local_array[k].word, temp_word) == 0)
                        {
                            local_array[k].count += count;
                            found = 1;
                            break;
                        }
                    }
                    if (found == 0)
                    {
                        local_array = (struct Frequency *)realloc(local_array, sizeof(struct Frequency) * (local_size + 1));
                        strcpy(local_array[local_size].word, temp_word);
                        local_array[local_size].count = count;
                        local_size++;
                    }
                }
            }
        }
        else
        {
            // send their own local_array size, and then loop send
            // their own word and count
            MPI_Send(&local_size, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            for (int i = 0; i < local_size; i++)
            {
                int temp = strlen(local_array[i].word) + 1;
                MPI_Send(&temp, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
                MPI_Send(local_array[i].word, 1, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
                MPI_Send(&local_array[i].count, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            }
        }
        if (!rank)
        {
            array = realloc(array, sizeof(struct Frequency) * size + 1);
            for (int i = 0; i < local_size; i++)
            {
                int found = 0;
                for (int j = 0; j < size; j++)
                    if (strcmp(local_array[i].word, array[j].word) == 0)
                    {
                        found = 1;
                        array[j].count += local_array[i].count;
                        break;
                    }
                if (found == 0)
                {
                    array = realloc(array, sizeof(struct Frequency) * (size + 1));
                    strcpy(array[size].word, local_array[i].word);
                    array[size].count = local_array[i].count;
                    size++;
                }
            }
        }
        printf("%lf \n", MPI_Wtime());
        fclose(infile);
        count++;
    }


    // arrange the array alphabetically or numerically
    if (output_order == 'a')
    {
        qsort(array, size, sizeof(struct Frequency), alphabetical);
    }
    else if (output_order == 'n')
    {
        qsort(array, size, sizeof(struct Frequency), numerical);
    }


    // print out to output.txt
    if (!rank)
    {
        outfile = fopen("output.txt", "w");
        if (!outfile)
        {
            printf("Unable to open output.txt for writing.\n");
            exit(1);
        }
        // fprintf(outfile, "Size: %d \n", size);


        fprintf(outfile, "Word Count Report (Number of Words Order):\n");


        for (int i = 0; i < size; i++)
        {
            if (strlen(array[i].word) >= minlen && strlen(array[i].word) <= maxlen)
                fprintf(outfile, "%s: %d \n", array[i].word, array[i].count);
        }


        fclose(outfile);
    }


    MPI_Finalize();
}
