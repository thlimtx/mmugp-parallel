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
    return structA->count - structB->count;
}


int main(int argc, char *argv[])
{
    int file_count = 0, minlen, maxlen, file_size, each_size;
    char output_order;
    char **file_name;
    char *file_content;
    char *each_content;
    FILE *infile;


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
    struct Frequency *array = (struct Frequency *)malloc(sizeof(struct Frequency) * 1);


    int count = 0;
    int size = 0;
    while (count < file_count)
    {
        if (!rank)
        {
            infile = fopen(file_name[count], "r");
            if (infile == NULL)
            {
                perror("Opening file");
                exit(1);
            }


            fseek(infile, 0, SEEK_END);
            file_size = ftell(infile);
            fseek(infile, 0, SEEK_SET);


            file_content = (char *)malloc(sizeof(char) * file_size);
            if (file_content == NULL)
            {
                perror("Empty Content");
                fclose(infile);
                exit(1);
            }


            fread(file_content, file_size, 1, infile);
            for (int i = 0; i < file_size; i++)
            {
                file_content[i] = tolower(file_content[i]);
            }
        }


        MPI_Bcast(&file_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
        each_size = file_size / np;
        each_content = (char *)malloc(sizeof(char) * (each_size + 1));
        MPI_Scatter(file_content, each_size, MPI_CHAR, each_content, each_size, MPI_CHAR, 0, MPI_COMM_WORLD);


        struct Frequency *each_array = (struct Frequency *)malloc(sizeof(struct Frequency) * 1);
        int temp_position = 0;


        char *s = " ,?.\t\n1234567890!@#$^&*()-_=+[]{}|;:\"\\<>/`~";
        char *word = strtok(each_content, s);


        while (word != NULL)
        {
            int check = 0;
            for (int i = 0; i < temp_position; i++)
            {
                if (strcmp(each_array[i].word, word) == 0)
                {
                    check = 1;
                    each_array[i].count++;
                    break;
                }
            }
            if (check == 0)
            {
                each_array = realloc(each_array, sizeof(struct Frequency) * (temp_position + 1));
                strcpy(each_array[temp_position].word, word);
                each_array[temp_position].count++;
                temp_position++;
            }
            word = strtok(NULL, s);
        }


        if (!rank)
        {
            for (int i = 1; i < np; i++)
            {
                int array_size = 0;
                MPI_Recv(&array_size, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
                printf("Rank %d: %d \n", i, array_size);
                printf("Rank %d: %d \n", rank, temp_position);
                for (int j = 0; j < array_size; j++)
                {
                    int temp, count, check = 0;


                    MPI_Recv(&temp, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
                    char *temp_word = malloc(sizeof(char) * temp);
                    MPI_Recv(temp_word, temp, MPI_CHAR, i, 1, MPI_COMM_WORLD, &status);
                    MPI_Recv(&count, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status);


                    for (int k = 0; k < temp_position; k++)
                    {
                        if (strcmp(each_array[k].word, temp_word) == 0)
                        {
                            each_array[k].count += count;
                            check = 1;
                            break;
                        }
                    }
                    if (check == 0)
                    {
                        each_array = (struct Frequency *)realloc(each_array, sizeof(struct Frequency) * (temp_position + 1));
                        strcpy(each_array[temp_position].word, temp_word);
                        each_array[temp_position].count = count;
                        temp_position++;
                    }
                }
            }
            // catch leftovers
            int leftover = file_size % np;
            char *temp = file_content + (file_size - leftover);
            char *word = strtok(temp, s);
            while (word != NULL)
            {
                int check = 0;
                for (int i = 0; i < temp_position; i++)
                {


                    if (strcmp(each_array[i].word, word) == 0)
                    {
                        check = 1;
                        each_array[i].count++;
                        break;
                    }
                }
                if (check == 0)
                {
                    each_array = realloc(each_array, sizeof(struct Frequency) * (size + 1));
                    strcpy(each_array[size].word, word);
                    each_array[size].count++;
                    temp_position++;
                }
                word = strtok(NULL, s);
            }
        }
        else
        {
            // send their own each_array size, and then loop send
            // their own word and count
            MPI_Send(&temp_position, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            for (int i = 0; i < temp_position; i++)
            {
                int temp = strlen(each_array[i].word) + 1;
                MPI_Send(&temp, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
                MPI_Send(each_array[i].word, 1, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
                MPI_Send(&each_array[i].count, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            }
        }


        // "working" till here


        if (!rank)
        {


            /*             for (int i = 0; i < 10; i++)
                        {
                            // if (strlen(each_array[i].word) >= minlen && strlen(each_array[i].word) <= maxlen)
                            printf("%s: %d \n", each_array[i].word, each_array[i].count);
                        }
            */


            array = realloc(array, sizeof(struct Frequency) * size + 1);
            for (int i = 0; i < temp_position; i++)
            {
                int check = 0;
                for (int j = 0; j < size; j++)
                    if (strcmp(each_array[i].word, array[j].word) == 0)
                    {
                        check = 1;
                        array[j].count += each_array[i].count;
                        break;
                    }
                if (check == 0)
                {
                    array = realloc(array, sizeof(struct Frequency) * (size + 1));
                    strcpy(array[size].word, each_array[i].word);
                    array[size].count = each_array[i].count;
                    size++;
                }
            }
            printf("Size: %d \n", size);
            /*
                        for (int i = 0; i < 20; i++)
                        {
                            printf("%s: %d \n", array[i].word, array[i].count);
                        }
             */


            fclose(infile);
        }
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


    if (!rank)
    {
        printf("Word Count Report (Number of Words Order):\n");


        for (int i = 0; i < 50; i++)
        {
            if (strlen(array[i].word) >= minlen && strlen(array[i].word) <= maxlen)
                printf("%s: %d \n", array[i].word, array[i].count);
        }
    }
    MPI_Finalize();
}
