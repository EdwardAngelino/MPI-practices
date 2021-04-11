/**
 * @author RookieHPC
 * @brief Original source code at https://www.rookiehpc.com/mpi/docs/mpi_type_create_struct.php
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

/**
 * @brief Illustrates how to create an indexed MPI datatype.
 * @details This program is meant to be run with 2 processes: a sender and a
 * receiver. These two MPI processes will exchange a message made of a
 * structure representing a person.
 *
 * Structure of a person:
 * - age: int
 * - height: double
 * - name: char[10]
 *
 * How to represent such a structure with an MPI struct:
 *
 *        +------------------ displacement for
 *        |         block 2: sizeof(int) + sizeof(double)
 *        |                         |
 *        +----- displacement for   |
 *        |    block 2: sizeof(int) |
 *        |            |            |
 *  displacement for   |            |
 *    block 1: 0       |            |
 *        |            |            |
 *        V            V            V
 *        +------------+------------+------------+
 *        |    age     |   height   |    name    |
 *        +------------+------------+------------+
 *         <----------> <----------> <---------->
 *            block 1      block 2      block 3
 *           1 MPI_INT  1 MPI_DOUBLE  10 MPI_CHAR
 **/

struct person_t
{
    int age;
    double height;
    double name[10];
};

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    // Get the number of processes and check only 2 processes are used
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if(size != 2)
    {
        printf("This application is meant to be run with 2 processes.\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Create the datatype
    MPI_Datatype person_type;
    int lengths[3] = { 1, 1, 10 };
    const MPI_Aint displacements[3] = { 0, sizeof(int), sizeof(int) + sizeof(double) };
    MPI_Datatype types[3] = { MPI_INT, MPI_DOUBLE, MPI_DOUBLE };
    MPI_Type_create_struct(3, lengths, displacements, types, &person_type);
    MPI_Type_commit(&person_type);

    // Get my rank and do the corresponding job
    enum rank_roles { SENDER, RECEIVER };
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    switch(my_rank)
    {
        case SENDER:
        {
            // Send the message
            struct person_t buffer[2];
            buffer[1].age = 20;
            buffer[1].height = 1.83;
            buffer[0].age = 21;
            buffer[0].height = 1.93;
           // strncpy(buffer.name, "Tom", 9);
           // buffer.name[9] = '\0';
            buffer[1].name[0] = 10.0; buffer[1].name[5] = 10.5; buffer[1].name[9] = 10.9;
            buffer[0].name[0] = 20.0; buffer[0].name[5] = 20.5; buffer[0].name[9] = 20.9;
            printf("MPI process %d sends person:\n\t- age0 = %d\n\t- height1 = %f\n\t- name 0,9= %.2f\n", my_rank, buffer[0].age, buffer[1].height, buffer[0].name[9]);
            MPI_Send(&buffer, 2, person_type, RECEIVER, 0, MPI_COMM_WORLD);
            break;
        }
        case RECEIVER:
        {
            // Receive the message
            struct person_t received[2];
            MPI_Recv(&received, 2, person_type, SENDER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("MPI process %d received person:\n\t- age1 = %d\n\t- height 0 = %f\n\t- name1,5 = %.2f\n",
                   my_rank, received[1].age, received[0].height, received[1].name[5]);
            break;
        }
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}

