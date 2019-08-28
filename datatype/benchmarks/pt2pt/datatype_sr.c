#define BENCHMARK "QX datatype Test"
#ifdef PACKAGE_VERSION
#   define HEADER "# " BENCHMARK " v" PACKAGE_VERSION "\n"
#else
#   define HEADER "# " BENCHMARK "\n"
#endif
/*
 * For detailed copyrighT:
 * QX,CAAD, BU
 * 01/25/2018
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//#include "papi.h"

#define IT 1000       //10000
#define ARRAY_SIZE 18  //initial array size being sent in the network, has to be times of 18 due to different datatype structures
#define STEP 50    //50 array_size*((step-1)*depth + 1) decides the biggest number of integers sent
#define DEPTH 50    //50
//#define MPI_IMP "OPEN MPI"
#define DEBUG 0    // debug flag
#define PAPI 0

struct particle
{
    char name[8];    //2 ints
    int x[3];        //3 ints
    char type[4];    //1 ints
};

struct bond{
    struct particle *H1;
    struct particle *O;
    float a,b,c;
};





int main(int argc, char **argv)
{
    int rank, size, i;
    MPI_Datatype type_cont, type_v, type_v_s2, type_stru_s, type_v_s4, type_stru, type_index, type_index_large;
    int sbuffer[400000];
    int rbuffer[400000];
    struct particle spar[100000];
    struct particle rpar[100000];
    MPI_Status status;
    MPI_Request request, request2;
    //***************for papi
    //float real_time, proc_time, mflops;
    //long long flpins;
    int retval, ev;
    long long values_s[4],values_e[4];
#if PAPI
    int EventSet = PAPI_NULL;
    int events[4]= { PAPI_L2_DCM, PAPI_L3_TCM, PAPI_TLB_DM};
#endif
    for (ev= 0; ev < 4; ev++){
        values_s[4] = 0;
        values_e[4] = 0;
    }
    long long v0[100][4],v1[100][4],v2[100][4],v3[100][4], v4[100][4],v5[100][4],v6[100][4],v7[100][4],v8[100][4];

    //*************************

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 2)
    {
        printf("Please run with 2 processes.\n");
        MPI_Finalize();
        return 1;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    char names[MPI_MAX_PROCESSOR_NAME];
    size_t len= MPI_MAX_PROCESSOR_NAME;
    gethostname( names, len );
    printf("#I am rank %d, I am running on node %s\n", rank,names);
    //* initialize the PAPI library
#if PAPI
    retval = PAPI_library_init(PAPI_VER_CURRENT);
    if(retval != PAPI_VER_CURRENT) {
        printf("PAPI init fail!\n");
        exit(1);
    }

    //* create an eventset
    if(PAPI_create_eventset(&EventSet)!= PAPI_OK){
        printf("PAPI create event fail\n");
        exit(1);
    }
    //* add an event
    if(PAPI_add_events(EventSet, events, 3)!= PAPI_OK){
        printf("PAPI add event fail\n");
        exit(1);
    }
#endif



    int size_iter;
    int array_size = ARRAY_SIZE;
    //check size times of 18 is possible
    if (array_size%18 != 0) {
        array_size = (array_size / 18) * 18;
        printf("array size has to be times of 18, changes to --> %d\n\n",array_size );
    }
    double t0[300],ts, te, t1[300], t2[300], t3[300], t4[300], t5[300], t6[300];

    double t7[300], t8[300];
    for (size_iter= 0; size_iter < STEP; size_iter++) {
        t0[size_iter] = 0.0f;
        t1[size_iter] = 0.0f;
        t2[size_iter] = 0.0f;
        t3[size_iter] = 0.0f;
        t4[size_iter] = 0.0f;
        t5[size_iter] = 0.0f;
        t6[size_iter] = 0.0f;
        t7[size_iter] = 0.0f;
        t8[size_iter] = 0.0f;
    }
    for (i=0; i<400000; i++)
    {
        sbuffer[i] = i;
        rbuffer[i] = i/2;
    }
    for (i = 0; i < 100000; i++){
        spar[i] = (struct particle){.name="abcdefgh", .x={i,i+1,i+2},.type="efg"};
        rpar[i] = (struct particle){.name="qingqing", .x={i+3,i+4,i+5},.type="qqx"};
    }

    int iteration = IT;
    int size_base = array_size;
    int count0 = size_base;      //int
    int count_index= size_base/3;
    int count2 = size_base /6;    //vector
    int count4 = size_base /6;    //struct
    int count6 = size_base /6;    //struct
    for (size_iter= 0; size_iter < STEP; size_iter= size_iter + 1) {
        //manipulating size, size is the same for everycases within this interation
        size_base = array_size * (DEPTH*size_iter+1);
        if (DEBUG) printf ("array size becomes %d\n", size_base);
        count0 = size_base;
        count_index = size_base/3;
        count2 = size_base /6;
        count4 = size_base /6;
        count6 = size_base /6;
        iteration = IT;
        //******type 1: contiguous type
        MPI_Type_contiguous(size_base, MPI_INT, &type_cont);   // int int int
        MPI_Type_commit(&type_cont);

        //******typp 2: long vector of int, with stride 2
        MPI_Type_vector(size_base, 1,1, MPI_INT,  &type_v_s2);  // (int *  )x n
        MPI_Type_commit(&type_v_s2);

        //******typp 3: long vector of int, with stride  aligned to cache block size (64B, 16 INT)
        MPI_Type_vector(size_base, 1,4, MPI_INT,  &type_v_s4);  // (int *  )x n
        MPI_Type_commit(&type_v_s4);

        //******typp 4: chopped vector of int, with stride 4
        MPI_Type_vector(6, 1,4, MPI_INT,  &type_v);  // (int * ** )x 6
        MPI_Type_commit(&type_v);

        MPI_Datatype type[3] = {MPI_CHAR, MPI_INT, MPI_CHAR};    // char * 8, int*3, char * 4
        int blocklen[3] = {8, 3, 4};
        MPI_Aint disp[3];

        MPI_Get_address(spar, disp);
        MPI_Get_address(spar[0].x, disp+1);
        MPI_Get_address(spar[0].type, disp+2);
        MPI_Aint base;
        base = disp[0];
        for (i=0; i < 3; i++) {
            disp[i] =  disp[i] - base;
        //        if (DEBUG) printf("disp: %d", disp[i]);
        }
        //*****type 5: struct type
        MPI_Type_create_struct(3, blocklen, disp, type, &type_stru);
        MPI_Type_commit(&type_stru);

        int blocklen2[4] = {1, 2, 2, 1};               // 1int* 2int** |2int ***1int   12 integer, 6 valid| new version: 6 integer, 3 valid
        int disp2[4] = {0, 2, 6, 11};

        //*****type 6: indexed type
        MPI_Type_indexed(4, blocklen2, disp2, MPI_INT, &type_index);
        MPI_Type_commit(&type_index);

        //*****type 7: big indexed type
        int blockcount =(int) (sqrt(1+8*size_base)/2 + 1/2);
        int * blocklen3;
        if (DEBUG) if (rank == 0) printf("black count is %d\n", blockcount);

        blocklen3 = (int *)malloc(blockcount * sizeof(int));
        int *disp3;
        disp3 = (int*) malloc(blockcount*sizeof(int));
        int iter_disp;
        for(iter_disp = 0; iter_disp < blockcount; iter_disp++){
            blocklen3[iter_disp] = iter_disp + 1;
            if (iter_disp != 0) {
                disp3[iter_disp] = disp3[iter_disp - 1] + iter_disp*2;
            }
            else {
                disp3[iter_disp] = 0;
            }
            //if(DEBUG) if (rank==0) printf("block len: %d,   disp:%d   \n", blocklen3[iter_disp], disp3[iter_disp]);
        }

        MPI_Type_indexed(blockcount, blocklen3, disp3, MPI_INT, &type_index_large);
        MPI_Type_commit(&type_index_large);

        //*****type 8: manually pack, unpack
        //*****type 9:mpi_type_create_indexed_block
        
        MPI_Barrier(MPI_COMM_WORLD);
        if (DEBUG) if (rank == 0) printf("iteration number is : %d\n", size_iter);
        //* PAPI start counting
#if PAPI
        if(PAPI_start(EventSet) != PAPI_OK){
            printf("PAPI cannot start counting\n");
            exit(1);
        }
#endif
            //blocking send and recv, rndv, with no data marshaling
            if (rank == 0)
            {
                for (iteration = 0;iteration<IT; iteration++){
                    if(iteration == 0) ts = MPI_Wtime();
                    MPI_Send(sbuffer, count0, MPI_INT, 1, 234, MPI_COMM_WORLD);
                    MPI_Recv(rbuffer, count0, MPI_INT, 1, 123, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                }
                te = MPI_Wtime();
            }

            if (rank == 1)
            {
                for (iteration = 0;iteration<IT; iteration++){
                    MPI_Recv(rbuffer, count0, MPI_INT, 0, 234, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                    MPI_Send(sbuffer, count0, MPI_INT, 0, 123, MPI_COMM_WORLD);
            
                }
            }

        //* PAPI read the counter
#if PAPI
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v0[size_iter][0] = values_e[0];
        v0[size_iter][1] = values_e[1];
        v0[size_iter][2] = values_e[2];
        //v0[size_iter][3] = values_e[3];
           

        if (rank ==0) t0[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE1, t1: %.6f\n", t0[size_iter]);
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
        //* PAPI read the counter
#if PAPI
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
            // blocking send & recv, contiguous datatype
            if (rank == 0)
            {
                for (iteration = 0;iteration<IT; iteration++){
                    if(iteration == 0) ts = MPI_Wtime();
                    MPI_Send(sbuffer, 1, type_cont, 1, 234, MPI_COMM_WORLD);
                    MPI_Recv(rbuffer, 1, type_cont, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                te = MPI_Wtime();
            }

            if (rank == 1)
            {
                for (iteration = 0;iteration<IT; iteration++){
                MPI_Recv(rbuffer, 1, type_cont, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(sbuffer, 1, type_cont, 0, 123, MPI_COMM_WORLD);
        
                }
            }

        //* PAPI read the counter
#if PAPI
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v1[size_iter][0] = values_e[0]-values_s[0];
        v1[size_iter][1] = values_e[1]-values_s[1];
        v1[size_iter][2] = values_e[2]-values_s[2];
        //v1[size_iter][3] = values_e[3]-values_s[3];
        if (rank==0) t1[size_iter] = (te - ts);

        if (DEBUG) if (rank == 0) printf("DONE2\n");
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
        //* PAPI read the counter
#if PAPI
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
            //blocking send and recv, rndv, with vector_s2 data marshaling
            if (rank == 0)
            {
                for (iteration = 0;iteration<IT; iteration++){
                    if(iteration == 0) ts = MPI_Wtime();
                    MPI_Send(sbuffer, 1, type_v_s2, 1, 234, MPI_COMM_WORLD);
                    MPI_Recv(rbuffer, 1, type_v_s2, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                te = MPI_Wtime();
            }

            if (rank == 1)
            {
                for (iteration = 0;iteration<IT; iteration++){
                    MPI_Recv(sbuffer, 1, type_v_s2, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Send(rbuffer, 1, type_v_s2, 0, 123, MPI_COMM_WORLD);
        
                }
            }

#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v2[size_iter][0] = values_e[0]-values_s[0];
        v2[size_iter][1] = values_e[1]-values_s[1];
        v2[size_iter][2] = values_e[2]-values_s[2];
        //v2[size_iter][3] = values_e[3]-values_s[3];
        if (rank ==0) t2[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE3\n");
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
            //blocking send and recv, rndv, with vector_s2 but replicated data marshaling
            if (rank == 0)
            {
                for (iteration = 0;iteration<IT; iteration++){
                    if(iteration == 0) ts = MPI_Wtime();
                    MPI_Send(sbuffer, count2, type_v, 1, 234, MPI_COMM_WORLD);
                    MPI_Recv(rbuffer, count2, type_v, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                te = MPI_Wtime();
            }

            if (rank == 1)
            {
                for (iteration = 0;iteration<IT; iteration++){
                    MPI_Recv(sbuffer, count2, type_v, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Send(rbuffer, count2, type_v, 0, 123, MPI_COMM_WORLD);
                }
            }

#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v8[size_iter][0] = values_e[0]-values_s[0];
        v8[size_iter][1] = values_e[1]-values_s[1];
        v8[size_iter][2] = values_e[2]-values_s[2];
        //v8[size_iter][3] = values_e[3]-values_s[3];
        if (rank == 0) t8[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE8\n");
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
            //blocking send and recv, rndv, with index lareg data marshaling
            if (rank == 0)
            {
                for (iteration = 0;iteration<IT; iteration++){
                    if(iteration == 0) ts = MPI_Wtime();
                    MPI_Send(sbuffer+iteration, 1, type_index_large, 1, 123, MPI_COMM_WORLD);
                    MPI_Recv(rbuffer+iteration, 1, type_index_large, 1, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                te = MPI_Wtime();

            }

            if (rank == 1)
            {
                for (iteration = 0;iteration<IT; iteration++){
                    MPI_Recv(sbuffer+iteration, 1, type_index_large, 0, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Send(rbuffer+iteration, 1, type_index_large, 0, 234, MPI_COMM_WORLD);
                }
            }


#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        free(disp3);
        free(blocklen3);
        MPI_Type_free(&type_index_large);
        v3[size_iter][0] = values_e[0]-values_s[0];
        v3[size_iter][1] = values_e[1]-values_s[1];
        v3[size_iter][2] = values_e[2]-values_s[2];
        if (rank==0) t3[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE4\n");
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
            //blocking send and recv, rndv, with vector_s16 data marshaling
            if (rank == 0)
            {
                for (iteration = 0;iteration<IT; iteration++){
                if(iteration == 0) ts = MPI_Wtime();
                MPI_Send(sbuffer, 1, type_v_s4, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, 1, type_v_s4, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                te = MPI_Wtime();
            }

            if (rank == 1)
            {
                for (iteration = 0;iteration<IT; iteration++){
                MPI_Recv(sbuffer, 1, type_v_s4, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, 1, type_v_s4, 0, 123, MPI_COMM_WORLD);
                }
            }
#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v7[size_iter][0] = values_e[0]-values_s[0];
        v7[size_iter][1] = values_e[1]-values_s[1];
        v7[size_iter][2] = values_e[2]-values_s[2];
        //v7[size_iter][3] = values_e[3]-values_s[3];
        if (rank == 0) t7[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE7\n");
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
            //blocking send and recv, rndv, with struct data marshaling
            if (rank == 0)
            {
                for (iteration = 0;iteration<IT; iteration++){
                //if(DEBUG) printf("count 4 is %d\n", count4);
                if(iteration == 0) ts = MPI_Wtime();
                MPI_Send(spar, count4, type_stru, 1, 123, MPI_COMM_WORLD);
                //if(DEBUG) printf("count 4 is %d\n", count4);
                MPI_Recv(rpar, count4, type_stru, 1, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                te = MPI_Wtime();
            }

            if (rank == 1)
            {
                for (iteration = 0;iteration<IT; iteration++){
                MPI_Recv(spar, count4, type_stru, 0, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rpar, count4, type_stru, 0, 234, MPI_COMM_WORLD);
                }
            }


#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v4[size_iter][0] = values_e[0]-values_s[0];
        v4[size_iter][1] = values_e[1]-values_s[1];
        v4[size_iter][2] = values_e[2]-values_s[2];
        //v4[size_iter][3] = values_e[3]-values_s[3];
        if (rank==0) t4[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE5\n");
        iteration = IT;
#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
            //blocking send and recv, rndv, with indexed data marshaling
            if (rank == 0)
            {
                for (iteration = 0;iteration<IT; iteration++){
                if(iteration == 0) ts = MPI_Wtime();
                MPI_Send(sbuffer, count2, type_index, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, count2, type_index, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                te = MPI_Wtime();
            }

            if (rank == 1)
            {
                for (iteration = 0;iteration<IT; iteration++){
                MPI_Recv(sbuffer, count2, type_index, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, count2, type_index, 0, 123, MPI_COMM_WORLD);
                }
            }

#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v5[size_iter][0] = values_e[0]-values_s[0];
        v5[size_iter][1] = values_e[1]-values_s[1];
        v5[size_iter][2] = values_e[2]-values_s[2];
        //v5[size_iter][3] = values_e[3]-values_s[3];
        if (rank==0) t5[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE3-index\n");
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;


        int count_buf = size_base*4;
        int counter0, counter1;
        int position_s = 0, position_r = 0;
        char *buffer_s;
        char *buffer_r;
        buffer_s = (char *) malloc(count_buf*2*sizeof(char));
        buffer_r = (char *) malloc(count_buf*2*sizeof(char));
        if (buffer_s == NULL) printf("buffers  malloc failed\n");
        if (buffer_r == NULL) printf("bufferr malloc failed\n");

        MPI_Barrier(MPI_COMM_WORLD);
#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        while (iteration > 0){
            position_s = 0;
            position_r = 0;

            if (DEBUG) if(rank == 0) printf("inner iteration of 6 is : %d\n", iteration);
            //blocking send and recv, rndv, with MPI_Pack()
            if (rank == 0)
            {
                if(DEBUG) printf("rank 0 says count 6 is %d\n", count4);
                if(iteration == IT) ts = MPI_Wtime();
                for (counter0 = 0 ; counter0 < count6; counter0++){
                    MPI_Pack(&spar[counter0], 8, MPI_CHAR, buffer_s, count_buf, &position_s, MPI_COMM_WORLD);
                    MPI_Pack(&(spar[counter0].x), 3, MPI_INT, buffer_s, count_buf, &position_s, MPI_COMM_WORLD);
                    MPI_Pack(&(spar[counter0].type), 4, MPI_CHAR, buffer_s, count_buf,&position_s, MPI_COMM_WORLD);
                    if (DEBUG)printf("rank0 pack results are: %d\n", spar[counter0].x[0]);
                }
                if(DEBUG) printf("rank 0 done packing\n");
                MPI_Send(buffer_s, count_buf, MPI_PACKED, 1, 123, MPI_COMM_WORLD);
                if(DEBUG) printf("rank 0 done sending\n");

                MPI_Recv(buffer_r, count_buf, MPI_PACKED, 1, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if(DEBUG) printf("rank0 has done receiving\n");
                for (counter0 = 0 ; counter0 < count6; counter0++){
                    MPI_Unpack(buffer_r, count_buf, &position_r, &(rpar[counter0]), 8, MPI_CHAR, MPI_COMM_WORLD);
                    MPI_Unpack(buffer_r, count_buf, &position_r, &(rpar[counter0].x), 3, MPI_INT, MPI_COMM_WORLD);
                    MPI_Unpack(buffer_r, count_buf, &position_r, &(rpar[counter0].type), 4, MPI_CHAR, MPI_COMM_WORLD);
                    if (DEBUG)printf("rank0 unpack results are: %d\n", rpar[counter0].x[0]);
                }
                if(DEBUG) printf("rank0 has doen packing\n");
            }

            if (rank == 1)
            {
                MPI_Recv(buffer_s, count_buf, MPI_PACKED, 0, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (DEBUG)printf("rank1 has done receiving, start unpacking\n");
                for (counter1 = 0 ; counter1 < count6; counter1++){
                    MPI_Unpack(buffer_s, count_buf, &position_s, &(spar[counter1]), 8, MPI_CHAR, MPI_COMM_WORLD);
                    MPI_Unpack(buffer_s, count_buf, &position_s, &(spar[counter1].x), 3, MPI_INT, MPI_COMM_WORLD);
                    MPI_Unpack(buffer_s, count_buf, &position_s, &(spar[counter1].type), 4, MPI_CHAR, MPI_COMM_WORLD);
                    if (DEBUG)printf("rank1 unpack results are: %d\n", spar[counter1].x[0]);
                }
                if (DEBUG)printf("rank1 has done unpacking\n");


                for (counter1 = 0 ; counter1 < count6; counter1++){
                    MPI_Pack(&rpar[counter1], 8, MPI_CHAR, buffer_r, count_buf, &position_r, MPI_COMM_WORLD);
                    MPI_Pack(&(rpar[counter1].x), 3, MPI_INT, buffer_r, count_buf, &position_r, MPI_COMM_WORLD);
                    MPI_Pack(&(rpar[counter1].type), 4, MPI_CHAR, buffer_r, count_buf, &position_r, MPI_COMM_WORLD);
                    if (DEBUG)printf("rank1 pack results are: %d\n", rpar[counter1].x[0]);
                }
                MPI_Send(buffer_r, count_buf, MPI_PACKED, 0, 234, MPI_COMM_WORLD);
                if(DEBUG) printf("rank1 has doen sending\n");

            }
            if (DEBUG) if (rank == 0) printf("done one iteration of 6\n");
            
            iteration--;
        }
            
        te = MPI_Wtime();
#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v6[size_iter][0] = values_e[0]-values_s[0];
        v6[size_iter][1] = values_e[1]-values_s[1];
        v6[size_iter][2] = values_e[2]-values_s[2];
        //v6[size_iter][3] = values_e[3]-values_s[3];
        if (rank==0) t6[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE6, t6: %.6f\n", t6[size_iter]);
        MPI_Barrier(MPI_COMM_WORLD);
#if PAPI
        PAPI_stop(EventSet, values_s);
#endif
        iteration = IT;

    }
    MPI_Barrier(MPI_COMM_WORLD);


    if (rank == 0)
    {
        printf("#BLK SEND & RECV MODE, MEASURE TIME OF PINGPONG(seconds)\n# --> ACTUAL {SENDING,PACKING}+{RECV,UNPACKING} TIME\n");
        printf("MPI IMPLEMENTATION, NON/BLK, ITERATION,SIZE,DATA_TYPE,WAIT TIME FOR SENDER, L1 D CACHE MISS, L2 D CACHE MISS, L3 TOTAL CACHE MISS, D TLB MISS \n");
        for (size_iter = 0; size_iter < STEP; size_iter=size_iter+1){
            //if (size_iter%2 == 0){
                printf(MPI_IMP",BLK, %d,%d,INT,%.6f, %d, %d, %d, %d\n", IT,array_size*(DEPTH*size_iter+1),t0[size_iter], v0[size_iter][0], v0[size_iter][1],v0[size_iter][2],v0[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,CONTIGUOUS,%.6f, %d, %d, %d, %d\n", IT,array_size*(DEPTH*size_iter+1), t1[size_iter], v1[size_iter][0], v1[size_iter][1],v1[size_iter][2],v1[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,VECTOR_S1,%.6f, %d, %d, %d, %d\n", IT,array_size*(DEPTH*size_iter+1), t2[size_iter], v2[size_iter][0], v2[size_iter][1],v2[size_iter][2],v2[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,VECTOR_S4,%.6f, %d, %d, %d, %d\n", IT,array_size*(DEPTH*size_iter+1), t7[size_iter], v7[size_iter][0], v7[size_iter][1],v7[size_iter][2],v7[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,VECTOR_S4 Replicate,%.6f, %d, %d, %d, %d\n", IT,array_size*(DEPTH*size_iter+1), t8[size_iter], v8[size_iter][0], v8[size_iter][1],v8[size_iter][2],v8[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,STRUCT,%.6f, %d, %d, %d, %d\n", IT,array_size*(DEPTH*size_iter+1), t4[size_iter], v4[size_iter][0], v4[size_iter][1],v4[size_iter][2],v4[size_iter][3]);
                //printf(MPI_IMP",BLK, %d,%d,STRUCT_S,%.6f\n", IT,array_size*(DEPTH*size_iter+1), t3[size_iter]);
                printf(MPI_IMP",BLK, %d,%d,INDEX Replicate,%.6f, %d, %d, %d, %d\n", IT,array_size*(DEPTH*size_iter+1), t5[size_iter], v5[size_iter][0], v5[size_iter][1],v5[size_iter][2],v5[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,INDEX,%.6f, %d, %d, %d, %d\n", IT,array_size*(DEPTH*size_iter+1), t3[size_iter], v3[size_iter][0], v3[size_iter][1],v3[size_iter][2],v3[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,PACK STRUCT,%.6f, %d, %d, %d, %d\n", IT,array_size*(DEPTH*size_iter+1), t6[size_iter], v6[size_iter][0], v6[size_iter][1],v6[size_iter][2],v6[size_iter][3]);
            //}
        }
        fflush(stdout);
    }
    MPI_Finalize();
    return 0;;
}

