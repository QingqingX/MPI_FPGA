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
#include <assert.h>

//#include "papi.h"

#define IT 1000       //10000
#define ARRAY_SIZE 10  //initial array size being sent in the network, has to be times of 18 due to different datatype structures
#define STEP 20    //50 array_size*((step-1)*depth + 1) decides the biggest number of integers sent
#define DEPTH 50    //50
//#define MPI_IMP "OPEN MPI"
#define DEBUG 0    // debug flag
#define PAPI 0
#define SKIP 10

void function(int size_iter, int sendbuf[], int recvbuf[], MPI_Datatype type,int size, double * avg_time, double * min_time, double * max_time){
    //for(size=options.min_message_size; size <= options.max_message_size; size *= 2) {
    int ranky;
    MPI_Comm_rank(MPI_COMM_WORLD, &ranky);
    MPI_Barrier(MPI_COMM_WORLD);
    double timer=0.0, t_start=0.0, t_stop=0.0, latency=0.0;
    *avg_time=0.0;
    *min_time=0.0;
    *max_time=0.0;
    if (DEBUG) if (ranky==0) printf("at iter # %d  inside function\n", size_iter);
         /*int stop =0;
         if (size_iter ==8){
         printf("PID %d ready for attach\n", getpid());
             fflush(stdout);
                 while (0 == stop)
                             sleep(5);}
        */
    //assert (sizeof(sendbuf)/sizeof(int) == size_iter*size_iter);
    int i;
    for (i=0; i < IT + SKIP ; i++) {
        t_start = MPI_Wtime();
        MPI_Alltoall(sendbuf, size, type, recvbuf, size, type,
                MPI_COMM_WORLD);
        t_stop = MPI_Wtime();

        if (i >= SKIP) {
            timer+=t_stop-t_start;
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    if (DEBUG) if (ranky==0) printf("at iter # %d  inside function, finished alltoall\n", size_iter);
    latency = (double)(timer * 1e6) / IT;

    MPI_Reduce(&latency, min_time, 1, MPI_DOUBLE, MPI_MIN, 0,
            MPI_COMM_WORLD);
    MPI_Reduce(&latency, max_time, 1, MPI_DOUBLE, MPI_MAX, 0,
            MPI_COMM_WORLD);
    MPI_Reduce(&latency, avg_time, 1, MPI_DOUBLE, MPI_SUM, 0,
            MPI_COMM_WORLD);
    int numprocs=0;
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    *avg_time = (*avg_time)/numprocs;

    MPI_Barrier(MPI_COMM_WORLD);
    return;
    //}
}

int main(int argc, char **argv)
{
    int rank, size, i;
    MPI_Datatype type_cont, type_v, type_v_s2, type_stru_s, type_v_s4, type_stru, type_index, type_index_large;


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
        printf("Please run with at least 2 processes.\n");
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
    double avg_time0[200], min_time0[200], max_time0[200];
    double avg_time1[200], min_time1[200], max_time1[200];
    double avg_time2[200], min_time2[200], max_time2[200];
    double avg_time3[200], min_time3[200], max_time3[200];

    double t7[300], t8[300];
    for (size_iter= 0; size_iter < STEP; size_iter++) {
        avg_time0[size_iter] = 0.0f;
        min_time0[size_iter] = 0.0f;
        max_time0[size_iter] = 0.0f;
        avg_time1[size_iter] = 0.0f;
        min_time1[size_iter] = 0.0f;
        max_time1[size_iter] = 0.0f;
        avg_time2[size_iter] = 0.0f;
        min_time2[size_iter] = 0.0f;
        max_time2[size_iter] = 0.0f;
        avg_time3[size_iter] = 0.0f;
        min_time3[size_iter] = 0.0f;
        max_time3[size_iter] = 0.0f;
    }


    int size_base;

    for (size_iter= 0; size_iter < STEP; size_iter= size_iter + 1) {
        //manipulating size, size is the same for everycases within this interation
        size_base = array_size * (size_iter+1);
	int matrix_size = size_base * size_base;
        if (DEBUG) printf ("array size becomes %d, matrix size is %d \n", size_base, matrix_size);
    	int sbuffer[matrix_size*size];
    	int rbuffer[matrix_size*size];
	int j;
        for (j =0; j < matrix_size; ++j){
            sbuffer[j] = (int)rand();
            rbuffer[j] = 0;
        }
        //******type 1: contiguous type
        MPI_Type_contiguous(matrix_size, MPI_INT, &type_cont);   // int int int
        MPI_Type_commit(&type_cont);

        //******typp 2: tiled: long vector of int, with stride  aligned to cache block size (64B, 16 INT)
        MPI_Type_vector(matrix_size/4, 1,4, MPI_INT,  &type_v);  // (int *  )x n
        MPI_Type_commit(&type_v);

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
        if (DEBUG) if (rank == 0) printf("block count is %d\n", blockcount);

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
        if (DEBUG) if (rank == 0) printf("at iter#: %d finished creating functions\n", size_iter);
        //* PAPI start counting
#if PAPI
        if(PAPI_start(EventSet) != PAPI_OK){
            printf("PAPI cannot start counting\n");
            exit(1);
        }
#endif

        MPI_Barrier(MPI_COMM_WORLD);
        //* PAPI read the counter
#if PAPI
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        // alltoall, contiguous datatype
        function(size_iter,sbuffer, rbuffer, type_cont, 1, &avg_time0[size_iter], &min_time0[size_iter], &max_time0[size_iter]);

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

        if (DEBUG) if (rank == 0) printf("DONE2 at iter# %d\n", size_iter);
        MPI_Barrier(MPI_COMM_WORLD);
        //* PAPI read the counter
#if PAPI
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        //blocking send and recv, rndv, with vector_s2 data marshaling
        function(size_iter,sbuffer, rbuffer, type_v, 1, &avg_time1[size_iter], &min_time1[size_iter], &max_time1[size_iter]);

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
        if (DEBUG) if (rank == 0) printf("DONE3\n");
        MPI_Barrier(MPI_COMM_WORLD);
#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        //blocking send and recv, rndv, with bucket but replicated data marshaling
        function(size_iter,sbuffer, rbuffer, type_index, 1, &avg_time2[size_iter], &min_time2[size_iter], &max_time2[size_iter]);

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
        if (DEBUG) if (rank == 0) printf("DONE8\n");
        MPI_Barrier(MPI_COMM_WORLD);
#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        //blocking send and recv, rndv, with index lareg data marshaling
        function(size_iter,sbuffer, rbuffer, type_index_large, 1, &avg_time3[size_iter], &min_time3[size_iter], &max_time3[size_iter]);

#if PAPI
        //* PAPI read the counter
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
        if (DEBUG) if (rank == 0) printf("DONE4\n");
        MPI_Barrier(MPI_COMM_WORLD);
#if PAPI
        PAPI_stop(EventSet, values_s);
#endif

    }
    //MPI_Barrier(MPI_COMM_WORLD);

    int psize = 0;
    if (rank == 0)
    {
        printf("#COLL MODE, MEASURE TIME OF CALL(seconds)\n# --> ACTUAL {COLL, PACKING}+{COLL, UNPACKING} TIME\n");
        if (MPI_IMP == "MVAPICH2")
		//printf("MPI IMPLEMENTATION, NON/BLK, ITERATION,SIZE,DATA_TYPE,WAIT TIME FOR SENDER, L1 D CACHE MISS, L2 D CACHE MISS, L3 TOTAL CACHE MISS, D TLB MISS \n");
		printf("MPI IMPLEMENTATION, NON/BLK, ITERATION,SIZE,DATA_TYPE,AV,MAX,MIN, L1 D CACHE MISS, L2 D CACHE MISS, L3 TOTAL CACHE MISS, D TLB MISS \n");
        for (size_iter = 0; size_iter < STEP; size_iter=size_iter+1){
            //if (size_iter%2 == 0){
		psize = array_size * (size_iter+1);
                printf(MPI_IMP",BLK, %d,%d,Contiguous,%.6f, %.6f, %.6f, %d, %d, %d, %d\n", IT,psize, avg_time0[size_iter], max_time0[size_iter], min_time0[size_iter],v1[size_iter][0], v1[size_iter][1],v1[size_iter][2],v1[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,Tiled,%.6f, %.6f, %.6f, %d, %d, %d, %d\n", IT,psize, avg_time1[size_iter], max_time1[size_iter], min_time1[size_iter], v2[size_iter][0], v2[size_iter][1],v2[size_iter][2],v2[size_iter][3]);
                //printf(MPI_IMP",BLK, %d,%d,STRUCT_S,%.6f\n", IT,array_size*(DEPTH*size_iter+1), t3[size_iter]);
                printf(MPI_IMP",BLK, %d,%d,Bucket,%.6f, %.6f, %.6f, %d, %d, %d, %d\n", IT,psize, avg_time2[size_iter], max_time2[size_iter], min_time2[size_iter], v3[size_iter][0], v3[size_iter][1],v3[size_iter][2],v3[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d,Alternating,%.6f, %.6f, %.6f, %d, %d, %d, %d\n", IT,psize, avg_time3[size_iter], max_time3[size_iter], min_time3[size_iter], v4[size_iter][0], v4[size_iter][1],v4[size_iter][2],v4[size_iter][3]);
            //}*/
        }
        fflush(stdout);
    }
    MPI_Finalize();
    return 0;;
}

