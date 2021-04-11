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
            struct person_t buffer;
            buffer.age = 20;
            buffer.height = 1.83;
           // strncpy(buffer.name, "Tom", 9);
           // buffer.name[9] = '\0';
            buffer.name[0] = 10.0; buffer.name[5] = 10.5; buffer.name[9] = 10.9;
            printf("MPI process %d sends person:\n\t- age = %d\n\t- height = %f\n\t- name 9= %.2f\n", my_rank, buffer.age, buffer.height, buffer.name[9]);
            MPI_Send(&buffer, 1, person_type, RECEIVER, 0, MPI_COMM_WORLD);
            break;
        }
        case RECEIVER:
        {
            // Receive the message
            struct person_t received;
            MPI_Recv(&received, 1, person_type, SENDER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("MPI process %d received person:\n\t- age = %d\n\t- height = %f\n\t- name = %.2f\n", my_rank, received.age, received.height, received.name[5]);
            break;
        }
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}

