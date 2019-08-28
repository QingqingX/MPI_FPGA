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
#include <unistd.h>
#include "papi.h"

#define IT 1000      //10000
#define ARRAY_SIZE 8000  //initial array size being sent in the network, has to be times of 18 due to different datatype structures
#define STEP 16    //50 array_size*((step-1)*depth + 1) decides the biggest number of integers sent
#define DEPTH 50    //50
#define SKIP 10
//#define BLOCKLEN 14
#define CPU_L1_CACHE 32768
//#define MPI_IMP "OPEN MPI"
#define DEBUG 0    // debug flag
#define PAPI 0
#define PAPIv 1    //enable for profiling cache miss diff btw v2 and v4 --> proving mem is not factor



int main(int argc, char **argv)
{
    int rank, size, i;
    MPI_Datatype type_cont, type_v2, type_v4, type_v8, type_v16, type_v32, type_v64;


    MPI_Status status;
    MPI_Request request, request2;
    //***************for papi
    float real_time, proc_time, mflops;
    long long flpins;
    int retval, ev;
    long long values_s[4],values_e[4];
#if PAPIv
    int EventSet = PAPI_NULL;
    int events[4]= {PAPI_L2_DCM, PAPI_L3_TCM, PAPI_TLB_DM, PAPI_TLB_DM};
#endif
    for (ev= 0; ev < 4; ev++){
        values_s[4] = 0;
        values_e[4] = 0;
    }
    long long v1[100][4],v2[100][4],v3[100][4], v4[100][4],v5[100][4],v6[100][4],v7[100][4];

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
        //* PAPI start counting
#if PAPIv
    if(PAPI_start(EventSet) != PAPI_OK){
        printf("PAPI cannot start counting\n");
        exit(1);
    }
#endif
        size_base = array_size;
        int matrix_size = size_base * size_base;
        int BLOCKLEN = 1;
    for (size_iter= 0; size_iter < STEP; size_iter= size_iter + 1) {
        //manipulating size, size is the same for everycases within this interation
        BLOCKLEN = size_iter+1;
        if (DEBUG) printf ("Blocklen becomes %d\n", size_base);
        int *sbuffer;
        int *mbuffer;
        int *rbuffer;
        int k1,k2,k3, counter = 0;
        k1 = posix_memalign((void **) &sbuffer, CPU_L1_CACHE, matrix_size * sizeof(int));
        if (k1!=0) {
            printf("Failed to allocate memory\n");
            exit(1);
        }
        k2 = posix_memalign((void **) &mbuffer, CPU_L1_CACHE, matrix_size * sizeof(int));
        if (k2!=0) {
            printf("Failed to allocate memory\n");
            exit(1);
        }
        k3 = posix_memalign((void **) &rbuffer, CPU_L1_CACHE, matrix_size * sizeof(int));
        if (k3!=0) {
            printf("Failed to allocate memory\n");
            exit(1);
        }
        int j;
        for (j =0; j < matrix_size; ++j){
            mbuffer[j] = (int)rand();
        }
        iteration = IT;
        //******type 1: contiguous type
        MPI_Type_contiguous((matrix_size/1024)*BLOCKLEN, MPI_INT, &type_cont);   // int int int
        MPI_Type_commit(&type_cont);
        //******typp 2: tiled: s2
        MPI_Type_vector(matrix_size/1024, BLOCKLEN,BLOCKLEN*2, MPI_INT,  &type_v2);  // (int *  )x n
        MPI_Type_commit(&type_v2);

        //******typp 3: tiled: long vector of int, with stride  aligned to cache block size (64B, 16 INT)
        MPI_Type_vector(matrix_size/1024, BLOCKLEN,BLOCKLEN*4, MPI_INT,  &type_v4);  // (int *  )x n
        MPI_Type_commit(&type_v4);

        //*****type 4: tiled, s8
        MPI_Type_vector(matrix_size/1024, BLOCKLEN,BLOCKLEN*8, MPI_INT,  &type_v8);  // (int *  )x n
        MPI_Type_commit(&type_v8);

        //*****type 5: tiled, s16
        MPI_Type_vector(matrix_size/1024, BLOCKLEN,BLOCKLEN*16, MPI_INT,  &type_v16);  // (int *  )x n
        MPI_Type_commit(&type_v16);

        //*****type 6: tiled, s32
        MPI_Type_vector(matrix_size/1024, BLOCKLEN,BLOCKLEN*32, MPI_INT,  &type_v32);  // (int *  )x n
        MPI_Type_commit(&type_v32);

        //*****type 6: tiled, s64
        MPI_Type_vector(matrix_size/1024, BLOCKLEN,BLOCKLEN*64, MPI_INT,  &type_v64);  // (int *  )x n
        MPI_Type_commit(&type_v64);

        MPI_Barrier(MPI_COMM_WORLD);
        if (DEBUG) if (rank == 0) printf("iteration number is : %d\n", size_iter);

        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
        //* PAPI read the counter
#if PAPIv
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
                MPI_Send(sbuffer, BLOCKLEN*matrix_size/1024, MPI_INT, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, BLOCKLEN*matrix_size/1024, MPI_INT, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
            MPI_Recv(rbuffer, BLOCKLEN*matrix_size/1024, MPI_INT, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(sbuffer, BLOCKLEN*matrix_size/1024, MPI_INT, 0, 123, MPI_COMM_WORLD);

            }
        }

        //* PAPI read the counter
#if PAPIv
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v1[size_iter][0] = values_e[0]-values_s[0];
        v1[size_iter][1] = values_e[1]-values_s[1];
        v1[size_iter][2] = values_e[2]-values_s[2];
        v1[size_iter][3] = values_e[3]-values_s[3];

        if (rank==0) t1[size_iter] = (te - ts);

        if (DEBUG) if (rank == 0) printf("DONE1 ori\n");
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
                MPI_Send(sbuffer, 1, type_v2, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, 1, type_v2, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                MPI_Recv(sbuffer, 1, type_v2, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, 1, type_v2, 0, 123, MPI_COMM_WORLD);

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
        v2[size_iter][3] = values_e[3]-values_s[3];
        if (rank ==0) t2[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE2\n");
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
                MPI_Send(sbuffer, 1, type_v4, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, 1, type_v4, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                MPI_Recv(sbuffer, 1, type_v4, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, 1, type_v4, 0, 123, MPI_COMM_WORLD);

            }
        }

#if PAPIv
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v3[size_iter][0] = values_e[0]-values_s[0];
        v3[size_iter][1] = values_e[1]-values_s[1];
        v3[size_iter][2] = values_e[2]-values_s[2];
        v3[size_iter][3] = values_e[3]-values_s[3];
        if (rank ==0) t3[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE3\n");
        // change to:  send the same aount of message time
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
        //* PAPI read the counter
#if PAPIv
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
                MPI_Send(sbuffer, 1, type_v8, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, 1, type_v8, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                MPI_Recv(sbuffer, 1, type_v8, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, 1, type_v8, 0, 123, MPI_COMM_WORLD);

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
        v4[size_iter][3] = values_e[3]-values_s[3];
        if (rank ==0) t4[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE NEW RUN-TIME TYPE \n");
        //////////////////////////DONE NEW RUNTIME TYPE
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
#if PAPIv
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
                MPI_Send(sbuffer, 1, type_v16, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, 1, type_v16, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                MPI_Recv(sbuffer, 1, type_v16, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, 1, type_v16, 0, 123, MPI_COMM_WORLD);
            }
        }

#if PAPIv
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v5[size_iter][0] = values_e[0]-values_s[0];
        v5[size_iter][1] = values_e[1]-values_s[1];
        v5[size_iter][2] = values_e[2]-values_s[2];
        v5[size_iter][3] = values_e[3]-values_s[3];
        if (rank == 0) t5[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE5\n");

        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
#if PAPIv
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
                MPI_Send(sbuffer, 1, type_v32, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, 1, type_v32, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                MPI_Recv(sbuffer, 1, type_v32, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, 1, type_v32, 0, 123, MPI_COMM_WORLD);
            }
        }

#if PAPIv
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v6[size_iter][0] = values_e[0]-values_s[0];
        v6[size_iter][1] = values_e[1]-values_s[1];
        v6[size_iter][2] = values_e[2]-values_s[2];
        v6[size_iter][3] = values_e[3]-values_s[3];
        if (rank == 0) t6[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE6\n");
        //v64
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;
#if PAPIv
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
                MPI_Send(sbuffer, 1, type_v64, 1, 123, MPI_COMM_WORLD);
                MPI_Recv(rbuffer, 1, type_v64, 1, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();

        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<(SKIP+IT); iteration++){
                MPI_Recv(sbuffer, 1, type_v64, 0, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuffer, 1, type_v64, 0, 234, MPI_COMM_WORLD);
            }
        }


#if PAPIv
        // PAPI read the counter
        if(PAPI_read(EventSet, values_e)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        v7[size_iter][0] = values_e[0]-values_s[0];
        v7[size_iter][1] = values_e[1]-values_s[1];
        v7[size_iter][2] = values_e[2]-values_s[2];
        v7[size_iter][3] = values_e[3]-values_s[3];
        if (rank==0) t7[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE7\n");
        free(sbuffer);
        free(mbuffer);
        free(rbuffer);
        MPI_Barrier(MPI_COMM_WORLD);
        iteration = IT;

    }
    MPI_Barrier(MPI_COMM_WORLD);
#if PAPIv
    PAPI_stop(EventSet, values_s);
#endif

    int psize = 0;
    double hard_size=0, hard_size_buk=0;
    if (rank == 0)
    {
        printf("#BLK SEND & RECV MODE, MEASURE TIME OF PINGPONG(seconds)\n# --> ACTUAL {SENDING,PACKING}+{RECV,UNPACKING} TIME\n");
	printf("MPI IMPLEMENTATION, NON/BLK, ITERATION,BLOCK LEN,DATA_TYPE,TIME FOR SENDER,BANDWIDTH(MB/s), L1 D CACHE MISS, L2 D CACHE MISS, L3 TOTAL CACHE MISS, D TLB MISS \n");
        for (size_iter = 0; size_iter < STEP; size_iter=size_iter+1){
            //if (size_iter%2 == 0){
                psize = (size_iter+1);
                hard_size = (size_iter+1)*matrix_size*(sizeof(int))/(1000*1024); //send actual kbytes
                printf(MPI_IMP",BLK, %d,%d,Contiguous,%.6f, %d, %d, %d, %d, %d\n", IT, psize, t1[size_iter], (int)(hard_size*2/t1[size_iter]), v1[size_iter][0],v1[size_iter][1],v1[size_iter][2],v1[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,Tiled_S2,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t2[size_iter], (int)(hard_size*2/(t2[size_iter]-t1[size_iter])), v2[size_iter][0],v2[size_iter][1],v2[size_iter][2],v2[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,Tiled_S4,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t3[size_iter], (int)(hard_size*2/(t3[size_iter]-t1[size_iter])), v3[size_iter][0],v3[size_iter][1],v3[size_iter][2],v3[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,Tiled_S8,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t4[size_iter], (int)(hard_size*2/(t4[size_iter]-t1[size_iter])), v4[size_iter][0],v4[size_iter][1],v4[size_iter][2],v4[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,Tiled_S16,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t5[size_iter], (int)(hard_size*2/(t5[size_iter]-t1[size_iter])), v5[size_iter][0],v5[size_iter][1],v5[size_iter][2],v5[size_iter][3]);
                //printf(MPI_IMP",BLK, %d,%d,STRUCT_S,%.6f\n", IT,array_size*(DEPTH*size_iter+1), t3[size_iter]);
                printf(MPI_IMP",BLK, %d,%d,Tiled_S32,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t6[size_iter], (int)(hard_size*2/(t6[size_iter]-t1[size_iter])), v6[size_iter][0],v6[size_iter][1],v6[size_iter][2],v6[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,Tiled_S64,%.6f, %d, %d, %d, %d, %d\n", IT,psize, t7[size_iter], (int)(hard_size*2/(t7[size_iter]-t1[size_iter])), v7[size_iter][0],v7[size_iter][1],v7[size_iter][2],v7[size_iter][3]);
                //printf(MPI_IMP",BLK, %d,%d,Alternating,%.6f, %d, %d, %d, %d\n", IT,psize, t4[size_iter], (int)(hard_size/t4[size_iter]), v4[size_iter][1],v4[size_iter][2],v4[size_iter][3]);
            //}
        }

        fflush(stdout);
    }
    MPI_Finalize();
    return 0;;
}

