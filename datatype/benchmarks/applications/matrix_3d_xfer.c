#define BENCHMARK "QX datatype Test -- 4 xferring only sveral columns of a matrix"
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

#define IT 100       //10000
#define MSIZE 10  //initial array size being sent in the network, has to be times of 18 due to different datatype structures

#define PARA  3
#define X     5
#define STEP 50    //50 array_size*((step-1)*depth + 1) decides the biggest number of integers sent
#define DEPTH 5    //50
//#define MPI_IMP "OPEN MPI"
#define DEBUG 0   // debug flag
#define PAPI 0

#define DATA1 double
//#define DATA2 float

#define MPI_DATA1 MPI_DOUBLE




int main(int argc, char **argv)
{
    int rank, size, i, count;
    MPI_Datatype type_v1, type_v2, type_v_col2, type_v_col1, type_struct, type_indexed1, type_indexed2, type_indexed_blk;
    MPI_Status status;
    MPI_Request request, request2;
	int msize;
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

    for (size_iter= 0; size_iter < STEP; size_iter= size_iter + 1) {
        //create matrix
        msize =  MSIZE * (size_iter+1);
        if (DEBUG) if (rank == 0) printf ("matrix size becomes %d   %d\n", msize, msize);
        count = X*msize*msize;
        int total = msize*msize*msize;
        iteration = IT;
		int i,j,p;
		DATA1 *smatrix = (DATA1 *)malloc(total* sizeof(DATA1));
		DATA1 *rmatrix = (DATA1 *)malloc(total* sizeof(DATA1));
    	// Note that arr[i][j] is same as *(*(arr+i)+j)
		for (p = 0; p <  total; p++){
		     		smatrix[p] = (DATA1)rand();  // OR *(*(arr+i)+j) = ++count
					rmatrix[p] = (DATA1)0;
		}

		if (DEBUG) if (rank == 0) printf("Created matrices\n");
        //******type 1: no type manually pack
        //MPI_Type_contiguous(size_base, MPI_DATA, &type_cont);   // int int int
        //MPI_Type_commit(&type_cont);

        //******typp 2: column is a vector of DATA with stride is M, blcoklength is X, blockcount is N, send M of these
        MPI_Type_vector(msize*msize, X,msize, MPI_DATA1,  &type_v_col1);  // (int *  )x n
        MPI_Type_commit(&type_v_col1);

		/* MPI_Type_vector(2, 1,msize*msize*msize, type_v_face1,  &type_v_col1);  // (int *  )x n */
        /* MPI_Type_commit(&type_v_col1); */
		/*MPI_Type_vector(msize, X,msize, MPI_DATA1,  &type_v2);  // (int *  )x n
        MPI_Type_commit(&type_v2);
		MPI_Type_vector(msize, 1,msize*msize, type_v2,  &type_v_col2);  // (int *  )x n
        MPI_Type_commit(&type_v_col2);
		*/
        //******typp 3: struct

        MPI_Datatype *type;
		int blockcount0 = msize*msize;
		type = (MPI_Datatype *)malloc(blockcount0 * sizeof(MPI_Datatype));
        int * blocklen0;
        blocklen0 = (int *)malloc(blockcount0 * sizeof(int));
        MPI_Aint *disp0;
        disp0 = (MPI_Aint*) malloc(blockcount0*sizeof(MPI_Aint));
        int iter_disp0;
		MPI_Get_address(smatrix, disp0);
		int out;
		MPI_Aint base = disp0[0];
                int it_disp;

		for (p = 0; p <  msize; p++){
			for (i = 0; i <  msize; i++){
                    iter_disp0 = i*msize + p*msize*msize;
                    it_disp = i + p*msize;
                    if(out == 0)
                            MPI_Get_address(&(smatrix[iter_disp0]), disp0+it_disp);
                    else
                            MPI_Get_address(&(smatrix[iter_disp0]), disp0+it_disp);
                    
				    disp0[it_disp] =  disp0[it_disp] - base;
			}
		}
		
	    for(iter_disp0 = 0; iter_disp0 < blockcount0; iter_disp0++){
	        blocklen0[iter_disp0] = X;
	    }


        for (iter_disp0 = 0; iter_disp0 < blockcount0; iter_disp0++){
			type[iter_disp0] = MPI_DATA1;
		}
		
if (DEBUG) if (rank == 0) printf("Created struct2\n");
        MPI_Type_create_struct(blockcount0, blocklen0, disp0, type, &type_struct);
if (DEBUG) if (rank == 0) printf("Created struct3\n");
        MPI_Type_commit(&type_struct);

		//*****type 4: indexed type

        int blockcount = msize*msize;
        int * blocklen;
        blocklen = (int *)malloc(blockcount * sizeof(int));
        int *disp;
        disp = (int*) malloc(blockcount*sizeof(int));
        int iter_disp;
		for (p = 0; p <  msize; p++){
			for (i = 0; i <  msize; i++){
				iter_disp = i + p*msize;
				if (iter_disp != 0) {
	            	disp[iter_disp] = disp[0] + msize*i + msize*msize*p;
			    }
			    else {
			        disp[iter_disp] = 0;
			    }
			}
		}


        for(iter_disp = 0; iter_disp < blockcount; iter_disp++){
            blocklen[iter_disp] = X;
        }


		MPI_Type_indexed(blockcount, blocklen, disp, MPI_DATA1, &type_indexed1);
        MPI_Type_commit(&type_indexed1);
		if (DEBUG) if (rank == 0) printf("Created indexed\n");

		//*****type 5: indexed_block type
/*  		int blockcount2 = msize;
        int *disp2;
        disp2 = (int*) malloc(blockcount2*sizeof(int));
        for(iter_disp = 0; iter_disp < blockcount2; iter_disp++){
            if (iter_disp != 0) {
                disp2[iter_disp] = disp2[iter_disp-1] + msize;
            }
            else {
                disp2[iter_disp] = 0;
            }
        }

        MPI_Type_create_indexed_block(blockcount2, X, disp2, MPI_DATA, &type_indexed_blk);
        MPI_Type_commit(&type_indexed_blk);
		if (DEBUG) if (rank == 0) printf("Created indexed block\n");
*/

#if PAPI
        if(PAPI_start(EventSet) != PAPI_OK){
            printf("PAPI cannot start counting\n");
            exit(1);
        }
#endif
        //blocking send and recv, rndv, with no data marshaling

		//create a contiguous buffer
		DATA1 send_buffer[msize*msize*X];
		DATA1 recv_buffer[msize*msize*X];
		
		count = msize*msize*X;
		int pack_iter,inner_iter;
		if (rank == 0)
        {

            for (iteration = 0;iteration<IT; iteration++){
                if(iteration == 0) ts = MPI_Wtime();
				for (pack_iter=0; pack_iter<msize*msize; ++pack_iter){
					for (inner_iter = 0; inner_iter < X; ++inner_iter){
						send_buffer[pack_iter*X+inner_iter] = smatrix[pack_iter*msize+inner_iter];
					}

				}
                MPI_Send(send_buffer, count, MPI_DATA1, 1, 234, MPI_COMM_WORLD);
				MPI_Recv(recv_buffer, count, MPI_DATA1, 1, 123, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
				for (pack_iter=0; pack_iter<msize*msize; ++pack_iter){
					for (inner_iter = 0; inner_iter < X; ++inner_iter){
						rmatrix[pack_iter*msize+inner_iter] = recv_buffer[pack_iter*X+inner_iter];
					}

				}
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<IT; iteration++){
                MPI_Recv(recv_buffer, count, MPI_DATA1, 0, 234, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
				for (pack_iter=0; pack_iter<msize*msize; ++pack_iter){
					for (inner_iter = 0; inner_iter < X; ++inner_iter){
						rmatrix[pack_iter*msize+inner_iter] = recv_buffer[pack_iter*X+inner_iter];
					}

				}
				for (pack_iter=0; pack_iter<msize*msize; ++pack_iter){
					for (inner_iter = 0; inner_iter < X; ++inner_iter){
						send_buffer[pack_iter*X+inner_iter] = smatrix[pack_iter*msize+inner_iter];
					}

				}
                MPI_Send(send_buffer, count, MPI_DATA1, 0, 123, MPI_COMM_WORLD);
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

        if (rank ==0) t0[size_iter] = (te - ts);
		int v,z,y,w;
		for (v=0; v < msize; v++){
			for (z=0; z < msize; z++){
				for (w=0; w < X; w++){
				y = w + msize*z + msize*msize*v;
				if (smatrix[y] != rmatrix[y]){
					printf("y is :  %d, manually copy data1 %lf %lf wrong\n", y, smatrix[y], rmatrix[y]);
					exit(1);
				}
			}
		}}

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
        if (DEBUG) if (rank == 0) printf("Start 2\n");
        // blocking send & recv, vector_col datatype
        if (rank == 0)
        {
            for (iteration = 0;iteration<IT; iteration++){
                if(iteration == 0) ts = MPI_Wtime();
                MPI_Send(smatrix, 1, type_v_col1, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rmatrix, 1, type_v_col1, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<IT; iteration++){
            MPI_Recv(rmatrix, 1, type_v_col1, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(smatrix, 1, type_v_col1, 0, 123, MPI_COMM_WORLD);
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
		for (v=0; v < msize; v++){
			for (z=0; z < msize; z++){
				for (w=0; w < X; w++){
				y = w + msize*z + msize*msize*v;
				if (smatrix[y] != rmatrix[y]){
					printf("v is :  %d, vector data1 %lf %lf wrong\n", v, smatrix[y], rmatrix[y]);
					exit(1);
				}
			}
		}}
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
        //blocking send and recv, rndv, with vector_row data marshaling
        if (rank == 0)
        {
            for (iteration = 0;iteration<IT; iteration++){
                if(iteration == 0) ts = MPI_Wtime();
                MPI_Send(smatrix, 1, type_struct, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rmatrix, 1, type_struct, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<IT; iteration++){
                MPI_Recv(smatrix, 1, type_struct, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rmatrix, 1, type_struct, 0, 123, MPI_COMM_WORLD);

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
		for (v=0; v < msize; v++){
			for (z=0; z < msize; z++){
				for (w=0; w < X; w++){
				y = w + msize*z + msize*msize*v;
				if (smatrix[y] != rmatrix[y]){
					printf("v is :  %d, struct data1 %lf %lf wrong\n", v, smatrix[y], rmatrix[y]);
					exit(1);
				}
			}
		}}
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
        if (DEBUG) if (rank == 0) printf("DONE31\n");
        //blocking send and recv, rndv, with indexed
        if (rank == 0)
        {
            for (iteration = 0;iteration<IT; iteration++){
                if(iteration == 0) ts = MPI_Wtime();
                MPI_Send(smatrix, 1, type_indexed1, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rmatrix, 1, type_indexed1, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<IT; iteration++){
                MPI_Recv(smatrix, 1, type_indexed1, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rmatrix, 1, type_indexed1, 0, 123, MPI_COMM_WORLD);
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

        if (rank == 0) t3[size_iter] = (te - ts);
		for (v=0; v < msize; v++){
			for (z=0; z < msize; z++){
				for (w=0; w < X; w++){
				y = w + msize*z + msize*msize*v;
				if (smatrix[y] != rmatrix[y]){
					printf("v is :  %d, indexed data1 %lf %lf wrong\n", v, smatrix[y], rmatrix[y]);
					exit(1);
				}
			}
		}
		}
        if (DEBUG) if (rank == 0) printf("DONE4\n");
        MPI_Barrier(MPI_COMM_WORLD);
/*

#if PAPI
        //* PAPI read the counter
        if(PAPI_read(EventSet, values_s)!= PAPI_OK){
            printf("PAPI cannot read counter value\n");
            exit(1);
        }
#endif
        //blocking send and recv, rndv, with indexed
        if (rank == 0)
        {
            for (iteration = 0;iteration<IT; iteration++){
                if(iteration == 0) ts = MPI_Wtime();
                MPI_Send(smatrix, 1, type_indexed_blk, 1, 234, MPI_COMM_WORLD);
                MPI_Recv(rmatrix, 1, type_indexed_blk, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            te = MPI_Wtime();
        }

        if (rank == 1)
        {
            for (iteration = 0;iteration<IT; iteration++){
                MPI_Recv(smatrix, 1, type_indexed_blk, 0, 234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rmatrix, 1, type_indexed_blk, 0, 123, MPI_COMM_WORLD);
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

        if (rank == 0) t4[size_iter] = (te - ts);
        if (DEBUG) if (rank == 0) printf("DONE4\n");

        MPI_Barrier(MPI_COMM_WORLD);
*/
#if PAPI
        PAPI_stop(EventSet, values_s);
#endif
        iteration = IT;
        	/* DATA1 *curr=smatrix; */
			/* free(curr); */

			/* curr=rmatrix; */
			/* free(curr); */

        free(smatrix);
        free(rmatrix);
    }
    MPI_Barrier(MPI_COMM_WORLD);


    if (rank == 0)
    {

        printf("#BLK SEND & RECV MODE, MEASURE TIME OF PINGPONG(seconds)\n# --> ACTUAL {SENDING,PACKING}+{RECV,UNPACKING} TIME\n");
        if (MPI_IMP == "MPICH"){
			printf("MPI IMPLEMENTATION, NON/BLK, ITERATION,MSIZE,msize, DATA_TYPE,WAIT TIME FOR SENDER, L1 D CACHE MISS, L2 D CACHE MISS, L3 TOTAL CACHE MISS, D TLB MISS \n");
		}
        for (size_iter = 0; size_iter < STEP; size_iter=size_iter+1){
			msize =  MSIZE * (size_iter+1);
			//msize =  msize * (size_iter+1);
            //if (size_iter%2 == 0){
                printf(MPI_IMP",BLK, %d,%d, %d,No_type,%.6f, %d, %d, %d, %d\n", IT,msize, msize,t0[size_iter], v0[size_iter][0], v0[size_iter][1],v0[size_iter][2],v0[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d, %d,Vector,%.6f, %d, %d, %d, %d\n", IT,msize, msize, t1[size_iter], v1[size_iter][0], v1[size_iter][1],v1[size_iter][2],v1[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d, %d,Struct,%.6f, %d, %d, %d, %d\n", IT,msize, msize, t2[size_iter], v2[size_iter][0], v2[size_iter][1],v2[size_iter][2],v2[size_iter][3]);
                printf(MPI_IMP",BLK, %d,%d, %d,Indexed,%.6f, %d, %d, %d, %d\n", IT,msize, msize, t3[size_iter], v3[size_iter][0], v3[size_iter][1],v3[size_iter][2],v3[size_iter][3]);
				//printf(MPI_IMP",BLK, %d,%d, %d,Indexed_blk,%.6f, %d, %d, %d, %d\n", IT,msize, msize, t4[size_iter], v4[size_iter][0], v4[size_iter][1],v4[size_iter][2],v4[size_iter][3]);

        }
        fflush(stdout);
    }
    MPI_Finalize();
    return 0;;
}

