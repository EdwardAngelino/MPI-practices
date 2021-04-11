#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

struct bar
{
    int a;
    int b;
    int* c;
};

void defineMPIType( MPI_Datatype* newType ) {
    struct bar tmp[2];
    MPI_Aint extent = &tmp[1] - &tmp[0];

    MPI_Type_create_resized( MPI_2INT, 0, extent, newType );
    MPI_Type_commit( newType );
}

int main( int argc, char* argv[] ) {
    MPI_Init(&argc, &argv);
    int rank, p;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &p );

    int cLen = atoi( argv[1] );
    MPI_Datatype Bar_type;
    defineMPIType( &Bar_type );

    struct bar msg;
    msg.c = ( int* ) malloc( sizeof( int ) * cLen );
    if ( rank == 0 ) {
        msg.a = 1;
        msg.b = 2;
        for ( int i = 0; i < cLen; ++i ) {
            msg.c[i] = i;
        }
        MPI_Send( &msg, 1, Bar_type, 1, 111, MPI_COMM_WORLD );
        MPI_Send( msg.c, cLen, MPI_INT, 1, 222, MPI_COMM_WORLD );
    }
    else if ( rank == 1 ) {
        MPI_Recv( &msg, 1, Bar_type, 0, 111, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
        MPI_Recv( msg.c, cLen, MPI_INT, 0, 222, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
    }

    printf("Rank %d has a = %d, b = %d, c = [", rank, msg.a, msg.b );
    for ( int i = 0; i < cLen - 1; ++i ) {
         printf( "%d, ", msg.c[i] );
    }
    printf( "%d]\n", msg.c[cLen - 1] );

    free( msg.c );
    MPI_Type_free( &Bar_type );
    MPI_Finalize();

    return 0;
}
