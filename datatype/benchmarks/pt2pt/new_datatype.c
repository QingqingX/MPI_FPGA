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
#include "papi.h"

#define IT 10000      //10000
#define ARRAY_SIZE 20  //initial array size being sent in the network, has to be times of 18 due to different datatype structures
#define STEP 50    //50 array_size*((step-1)*depth + 1) decides the biggest number of integers sent
#define DEPTH 50    //50
#define SKIP 10
//#define MPI_IMP "OPEN MPI"
#define DEBUG 0    // debug flag
#define PAPI 0
#define PAPIv 1    //enable for profiling cache miss diff btw v2 and v4 --> proving mem is not factor

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
    MPI_Datatype type_cont, type_v, type_v_run, type_v_s2, type_stru_s, type_v_s4, type_stru, type_index, type_index_large;


    MPI_Status status;
    MPI_Request request, request2;
    //***************for papi
    float real_time, proc_time, mflops;
    long long flpins;
    int retval, ev;
    long long values_s[4],values_e[4];
#if PAPIv
    int EventSet = PAPI_NULL;
    int events[4]= {PAPI_L1_DCM, PAPI_L2_DCM, PAPI_L3_TCM, PAPI_TLB_DM};
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
#if PAPIv
    retval = PAPI_library_init(PAPI_VER_CURRENT);
    printf("PAPI retval , %d\n", retval);
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


    int iteration = IT;
    int size_base;

    for (size_iter= 0; size_iter < STEP; size_iter= size_iter + 1) {
        //manipulating size, size is the same for everycases within this interation
        size_base = array_size * (size_iter+1);
        int matrix_size = size_base * size_base;
        if (DEBUG) printf ("array size becomes %d\n", size_base);
    	int sbuffer[matrix_size];
    	int mbuffer[matrix_size];
    	int rbuffer[matrix_size];
        int j;
        for (j =0; j < matrix_size; ++j){
            mbuffer[j] = (int)rand();
        }
        iteration = IT;
        //******type 1: contiguous type
        MPI_Type_contiguous(matrix_size, MPI_INT, &type_cont);   // int int int
        MPI_Type_commit(&type_cont);

        //******typp 2: tiled: long vector of int, with stride  aligned to cache block size (64B, 16 INT)
        MPI_Type_vector(matrix_size/4, 1,4, MPI_INT,  &type_v);  // (int *  )x n
        MPI_Type_commit(&type_v);
        //******typp 5: tiled: long vector of int, with stride  aligned to cache block size (64B, 16 INT)
        MPI_Type_vector(matrix_size/4, 1,2, MPI_INT,  &type_v_s2);  // (int *  )x n
        MPI_Type_commit(&type_v_s2);

        //*****type 3: bucket, indexed type, the triangle of a matrix
        int blocklen[size_base];
        int disp[size_base];
        int k;
        for (k=0; k < size_base; ++k){
            blocklen[k] = size_base - k;
            disp[k] = size_base*k;
        }

        MPI_Type_indexed(size_base, blocklen, disp, MPI_INT, &type_index);
        MPI_Type_commit(&type_index);


        //*****type 4: alternating, big indexed type
        int blockcount = size_base;
        int * blocklen2;
        if (DEBUG) if (rank == 0) printf("black count is %d\n", blockcount);

        blocklen2 = (int *)malloc(blockcount * sizeof(int));
        int *disp2;
        disp2 = (int*) malloc(blockcount*sizeof(int));
        int iter_disp;
        for(iter_disp = 0; iter_disp < blockcount; iter_disp++){
            blocklen2[iter_disp] = iter_disp + 1;
            if (iter_disp != 0) {
                disp2[iter_disp] = disp2[iter_disp - 1] + iter_disp*2;
            }
            else {
                disp2[iter_disp] = 0;
            }
            //if(DEBUG) if (rank==0) printf("block len: %d,   disp:%d   \n", blocklen3[iter_disp], disp3[iter_disp]);
        }

        MPI_Type_indexed(blockcount, blocklen2, disp2, MPI_INT, &type_index_large);
        MPI_Type_commit(&type_index_large);




        MPI_Barrier(MPI_COMM_WORLD);
        if (DEBUG) if (rank == 0) printf("iteration number is : %d\n", size_iter);
        //* PAPI start counting
#if PAPIv
        if(PAPI_start(EventSet) != PAPI_OK){
            printf("PAPI cannot start counting\n");
            exit(1);
        }
#endif

        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
        //* PAPI read the counter
#if PAPI
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        //touch data to avoid cache cold start
        for (j =0; j < matrix_size; ++j){
            sbuffer[j] = mbuffer[j];
            rbuffer[j] = 0;
        }
        // blocking send & recv, contiguous datatype
        if (rank == 0)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                if(iteration == SKIP) ts = MPI_Wtime();
                MPI_Send(sbuffer, 1, type_cont, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, 1, type_cont, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
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
        //touch data to avoid cache cold start
        for (j =0; j < matrix_size; ++j){
            sbuffer[j] = mbuffer[j];
            rbuffer[j] = 0;
        }
        // blocking send & recv, contiguous without
        if (rank == 0)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                if(iteration == SKIP) ts = MPI_Wtime();
                MPI_Send(sbuffer, matrix_size, MPI_INT, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, matrix_size, MPI_INT, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
            MPI_Recv(rbuffer, matrix_size, MPI_INT, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(sbuffer, matrix_size, MPI_INT, 0, 123, MPI_COMM_WORLD);

            }
        }

        //* PAPI read the counter
#if PAPI
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif

        if (rank==0) t6[size_iter] = (te - ts);

        if (DEBUG) if (rank == 0) printf("DONE2 ori\n");
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
        //touch data to avoid cache cold start
        for (j =0; j < matrix_size; ++j){
            sbuffer[j] = mbuffer[j];
            rbuffer[j] = 0;
        }
        //* PAPI read the counter
#if PAPIv
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        //blocking send and recv, rndv, with vector_s2 data marshaling
        if (rank == 0)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                if(iteration == SKIP) ts = MPI_Wtime();
                MPI_Send(sbuffer, 1, type_v, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, 1, type_v, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                MPI_Recv(sbuffer, 1, type_v, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, 1, type_v, 0, 123, MPI_COMM_WORLD);

            }
        }

#if PAPIv
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
        //type_v_s2~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
        //touch data to avoid cache cold start
        for (j =0; j < matrix_size; ++j){
            sbuffer[j] = mbuffer[j];
            rbuffer[j] = 0;
        }
        //* PAPI read the counter
#if PAPIv
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        //blocking send and recv, rndv, with vector_s2 data marshaling
        if (rank == 0)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                if(iteration == SKIP) ts = MPI_Wtime();
                MPI_Send(sbuffer, 1, type_v_s2, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, 1, type_v_s2, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                MPI_Recv(sbuffer, 1, type_v_s2, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, 1, type_v_s2, 0, 123, MPI_COMM_WORLD);

            }
        }

#if PAPIv
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v4[size_iter][0] = values_e[0]-values_s[0];
        v4[size_iter][1] = values_e[1]-values_s[1];
        v4[size_iter][2] = values_e[2]-values_s[2];
        //v2[size_iter][3] = values_e[3]-values_s[3];
        if (rank ==0) t4[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE4\n");
        ///////////////////////new type for profiling runtime packing
        ///////////////////////does not work
        // change to:  send the same aount of message time
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
        //* PAPI read the counter
#if PAPI
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        //touch data to avoid cache cold start
        for (j =0; j < matrix_size; ++j){
            sbuffer[j] = mbuffer[j];
            rbuffer[j] = 0;
        }
        if (rank == 0)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                if(iteration == SKIP) ts = MPI_Wtime();
                MPI_Send(sbuffer, matrix_size/4, MPI_INT, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, matrix_size/4, MPI_INT, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                MPI_Recv(sbuffer, matrix_size/4, MPI_INT, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, matrix_size/4, MPI_INT, 0, 123, MPI_COMM_WORLD);

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
        if (rank ==0) t5[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE NEW RUN-TIME TYPE \n");
        //////////////////////////DONE NEW RUNTIME TYPE
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        //touch data to avoid cache cold start
        for (j =0; j < matrix_size; ++j){
            sbuffer[j] = mbuffer[j];
            rbuffer[j] = 0;
        }
        //blocking send and recv, rndv, with bucket but replicated data marshaling
        if (rank == 0)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                if(iteration == SKIP) ts = MPI_Wtime();
                MPI_Send(sbuffer, 1, type_index, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, 1, type_index, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                MPI_Recv(sbuffer, 1, type_index, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, 1, type_index, 0, 123, MPI_COMM_WORLD);
            }
        }

#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v3[size_iter][0] = values_e[0]-values_s[0];
        v3[size_iter][1] = values_e[1]-values_s[1];
        v3[size_iter][2] = values_e[2]-values_s[2];
        //v8[size_iter][3] = values_e[3]-values_s[3];
        if (rank == 0) t3[size_iter] = (te - ts);
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
        //blocking send and recv, rndv, with bucket but replicated data marshaling
        int hardsize_bucket = (1+size_base)*size_base/2;
        //touch data to avoid cache cold start
        for (j =0; j < matrix_size; ++j){
            sbuffer[j] = mbuffer[j];
            rbuffer[j] = 0;
        }
        if (rank == 0)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                if(iteration == SKIP) ts = MPI_Wtime();
                MPI_Send(sbuffer, hardsize_bucket, MPI_INT, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, hardsize_bucket, MPI_INT, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                MPI_Recv(sbuffer, hardsize_bucket, MPI_INT, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, hardsize_bucket, MPI_INT, 0, 123, MPI_COMM_WORLD);
            }
        }

#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v3[size_iter][0] = values_e[0]-values_s[0];
        v3[size_iter][1] = values_e[1]-values_s[1];
        v3[size_iter][2] = values_e[2]-values_s[2];
        //v8[size_iter][3] = values_e[3]-values_s[3];
        if (rank == 0) t7[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE8_ori\n");
        /* alternating type is removed for FTP
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
#if PAPI
        // PAPI read the counter
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        //blocking send and recv, rndv, with index lareg data marshaling
        if (rank == 0)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                if(iteration == SKIP) ts = MPI_Wtime();
                MPI_Send(sbuffer, 1, type_index_large, 1, 123, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, 1, type_index_large, 1, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();

        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                MPI_Recv(sbuffer, 1, type_index_large, 0, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, 1, type_index_large, 0, 234, MPI_COMM_WORLD);
            }
        }


#if PAPI
        // PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        free(disp2);
        free(blocklen2);
        MPI_Type_free(&type_index_large);
        v4[size_iter][0] = values_e[0]-values_s[0];
        v4[size_iter][1] = values_e[1]-values_s[1];
        v4[size_iter][2] = values_e[2]-values_s[2];
        if (rank==0) t4[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE4\n");
        */
        MPI_Barrier(MPI_COMM_WORLD);
#if PAPIv
        PAPI_stop(EventSet, values_s);
#endif
        iteration = IT;

    }
    MPI_Barrier(MPI_COMM_WORLD);

	int psize = 0;
        double hard_size=0, hard_size_buk=0;
    if (rank == 0)
    {
        printf("#BLK SEND & RECV MODE, MEASURE TIME OF PINGPONG(seconds)\n# --> ACTUAL {SENDING,PACKING}+{RECV,UNPACKING} TIME\n");
        if (MPI_IMP == "OPEN MPI")
			printf("MPI IMPLEMENTATION, NON/BLK, ITERATION,SIZE,DATA_TYPE,TIME FOR SENDER,BANDWIDTH(MB/s), L1 D CACHE MISS, L2 D CACHE MISS, L3 TOTAL CACHE MISS, D TLB MISS \n");
        for (size_iter = 0; size_iter < STEP; size_iter=size_iter+1){
            //if (size_iter%2 == 0){
                psize = array_size * (size_iter+1);
                hard_size = 2*psize*psize*(sizeof(int))/4000; //kbytes
                hard_size_buk = 2*(1+psize)*psize*(sizeof(int))/2000; //kbytes
                printf(MPI_IMP",BLK, %d,%d,Contiguous,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t1[size_iter], (int)(hard_size*40/t1[size_iter]), v1[size_iter][0],v1[size_iter][1],v1[size_iter][2],v1[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,EQL_Contiguous_NOM,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t6[size_iter], (int)(hard_size*40/t6[size_iter]), v6[size_iter][0],v6[size_iter][1],v6[size_iter][2],v6[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,Tiled_S2,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t4[size_iter], (int)(hard_size*10/t4[size_iter]), v4[size_iter][0],v4[size_iter][1],v4[size_iter][2],v4[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,Tiled,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t2[size_iter], (int)(hard_size*10/t2[size_iter]), v2[size_iter][0],v2[size_iter][1],v2[size_iter][2],v2[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,EQL_Tiled_NOM,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t5[size_iter], (int)(hard_size*10/t5[size_iter]), v5[size_iter][0],v5[size_iter][1],v5[size_iter][2],v5[size_iter][3]);
                //printf(MPI_IMP",BLK, %d,%d,STRUCT_S,%.6f\n", IT,array_size*(DEPTH*size_iter+1), t3[size_iter]);
                printf(MPI_IMP",BLK, %d,%d,Bucket,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t3[size_iter], (int)(hard_size_buk*10/t3[size_iter]), v3[size_iter][0],v3[size_iter][1],v3[size_iter][2],v3[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,EQL_Bucket_NOM,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t7[size_iter], (int)(hard_size_buk*10/t7[size_iter]), v7[size_iter][0],v7[size_iter][1],v7[size_iter][2],v7[size_iter][3]);
                //printf(MPI_IMP",BLK, %d,%d,Alternating,%.6f, %d, %d, %d, %d\n", IT,psize, t4[size_iter], (int)(hard_size/t4[size_iter]), v4[size_iter][1],v4[size_iter][2],v4[size_iter][3]);
            //}
        }
        fflush(stdout);
    }
    MPI_Finalize();
    return 0;;
}

