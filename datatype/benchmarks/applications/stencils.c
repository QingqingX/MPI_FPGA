#define BENCHMARK "QX stencil Test"
#ifdef PACKAGE_VERSION
#   define HEADER "# " BENCHMARK " v" PACKAGE_VERSION "\n"
#else
#   define HEADER "# " BENCHMARK "\n"
#endif
/*
 * For detailed copyrighT:
 * QX,CAAD, BU
 * This is mainly an rewrite of the stencil application
 * We use this for testing datatype's impact in stencil,
 * as well as testing new data marhsaling ideas (packing close to cache)
 * 01/25/2018
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define IT 100
#define ARRAY_SIZE 16//16  //one dimensional size of matrices
#define OUT_SIZE 10  //2 bigger than ARRAY_SIZE
#define N 3    //rank numebr in each dimension
#define STEP 35//20
//#define MPI_IMP "OPEN MPI"
#define DEBUG 0    // debug flag

#define matrix(a, b, c) matrix[c+(b)*OUT_SIZE+(a)*OUT_SIZE*OUT_SIZE]
void create_neighbor(int * nb, int rank){
    int size = ARRAY_SIZE * ARRAY_SIZE;
    //has front neighbor
    if(rank >= N * N){    //rank ID = x + Ny + N*Nz
        nb[0] = rank - N*N;
    }
    //has back neighbor
    if(rank < (N*N*N - N*N)){
        nb[1] = rank + N*N;
    }
    //has left neighbor
    if(rank % N != 0){
        nb[2] = rank - 1;
    }
    //has right neighbor
    if(rank % N != (N-1)){
        nb[3] = rank + 1;
    }
    //has up neighbor
    if((rank % (N*N)) >= N ){
        nb[4] = rank - N;
    }
    //has down neighbor
    if((rank % (N*N)) < (N*N-N) ){
        nb[5] = rank + N;
    }
}




int main(int argc, char **argv)
{
    int rank, size, i, index;
    MPI_Datatype type_row, type_col, type_ud, type_lr, type_index;

    MPI_Status status[12];
    MPI_Request request[12];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 2)
    {
        printf("Please run with 2 processes.\n");
        MPI_Finalize();
        return 1;
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int myself = rank;
    int nb[6];
    for (i=0; i<6;++i){
        nb[i] = -1;
    }
    create_neighbor(nb, myself);
    if (DEBUG){
        if(rank == 2){
            printf("I am rank :%d, my neighbors are:  \n", myself);
            for (i=0; i<6;++i){
                printf("%d\t", nb[i]);
            }
            printf("\n");
        }
    }
    char names[MPI_MAX_PROCESSOR_NAME];
    size_t len= MPI_MAX_PROCESSOR_NAME;
    gethostname( names, len );
    printf("#I am rank %d, I am running on node %s\n", rank,names);


    //******type 1: manually copy to send buffer , no pack


    //*****type 2: vector of double, with stride of



    //*****type 3: OP1

    double t0[300],ts, tm, tmm, te, t1[300],t0m[300],t1m[300],t2[300],t2m[300];
    int size_iter, arraysize, count, outsize;
    for (size_iter= 0; size_iter < STEP; size_iter++) {
        t0[size_iter] = 0.0f;
        t1[size_iter] = 0.0f;
        t0m[size_iter] = 0.0f;
        t1m[size_iter] = 0.0f;
    t2[size_iter] = 0.0f;
        t2m[size_iter] = 0.0f;
    }

    int iteration = IT;
    int send_count, recv_count;
    for (size_iter=0 ; size_iter < STEP; size_iter++){
        arraysize = ARRAY_SIZE * (size_iter+1);
        outsize = arraysize+2;
        int count = arraysize * arraysize;
                if(DEBUG) if (rank==0) printf("start sending arraysize: %d, count size:%d\n", arraysize, count);

        //create a 3D matrix to each rank, 2D is column major, z-dimention is the last ordered matrix[z][y][x]
          int it1, it2, it3;
                  /* cannot malloc like this! not consecutive in memory
          double ***matrix = (double***)malloc(outsize*sizeof(double**))
                  */


                double *matrix = (double*) malloc(outsize*outsize*outsize*sizeof(double));

          for (it1 = 0; it1 < outsize; it1++) {
            for (it2 = 0; it2 < outsize; it2++) {
            for (it3 = 0; it3 < outsize; it3++) {
                    matrix(it1,it2,it3) = (double)rand();
                    //if(DEBUG) if((rank==0)&& (size_iter == 0)) printf("%d\t",matrix(it1,it2,it3) );
                }
            }
          }
        MPI_Barrier(MPI_COMM_WORLD);
        ts = MPI_Wtime();
                //double matrix[outsize][outsize][outsize];   //actual size is 16x16x16, one ghost circle for storing neighbors' results
        //create contiguous buffer for each direction
        double *front, *back, *left, *right, *up, *down, *rfront, *rback, *rleft, *rright, *rup, *rdown;
        double *newfront, *back2, *left2, *right2, *up2, *down2, *rnewfront, *rback2, *rleft2, *rright2, *rup2, *rdown2;
        front = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (front == NULL) printf("front malloc failed\n");
        back = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (back == NULL) printf("back malloc failed\n");
        left = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (left == NULL) printf("left malloc failed\n");
        right = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (right == NULL) printf("front malloc failed\n");
        up = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (up == NULL) printf("front malloc failed\n");
        down = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (down == NULL) printf("front malloc failed\n");
        rfront = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (rfront == NULL) printf("front malloc failed\n");
        rback = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (rback == NULL) printf("front malloc failed\n");
        rleft = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (rleft == NULL) printf("front malloc failed\n");
        rright = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (rright == NULL) printf("front malloc failed\n");
        rup = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (rup == NULL) printf("front malloc failed\n");
        rdown = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (rdown == NULL) printf("front malloc failed\n");

                int datatype_size;
                iteration = IT;
                MPI_Type_size(MPI_DOUBLE, &datatype_size);
                if(DEBUG) if(rank==0) printf("double size is : %d, and MPI_DOUBLE size is : %d \n", sizeof(double), datatype_size);

        while (iteration > 0){

                    tm = MPI_Wtime();
                    recv_count=0;
                    send_count=0;
                    index = -1;
                    int req_iter;
                    for (req_iter = 0; req_iter<12; req_iter++) request[req_iter] = MPI_REQUEST_NULL;
            //step 1 irecv from neighbors
            //has front neighbor
            if(nb[0] != -1){
                        MPI_Irecv(rfront, count, MPI_DOUBLE, nb[0], 1, MPI_COMM_WORLD,&request[0]);
                                recv_count++;
            }
            //has back neighbor
            if(nb[1] != -1){
                MPI_Irecv(rback, count, MPI_DOUBLE, nb[1], 2, MPI_COMM_WORLD,&request[1]);
                                recv_count++;
            }
            //has left neighbor
            if(nb[2] != -1){
                MPI_Irecv(rleft, count, MPI_DOUBLE, nb[2], 3, MPI_COMM_WORLD,&request[2]);
                                recv_count++;
            }
            //has right neighbor
            if(nb[3] != -1){
                MPI_Irecv(rright, count, MPI_DOUBLE, nb[3], 4, MPI_COMM_WORLD,&request[3]);
                                recv_count++;
            }
            //has up neighbor
            if(nb[4] != -1){
                MPI_Irecv(rup, count, MPI_DOUBLE, nb[4], 5, MPI_COMM_WORLD,&request[4]);
                                recv_count++;
            }
            //has down neighbor
            if(nb[5] != -1){
                MPI_Irecv(rdown, count, MPI_DOUBLE, nb[5], 6, MPI_COMM_WORLD,&request[5]);
                                recv_count++;
            }
                        if(DEBUG) if (rank ==0)printf("Done posting Irecvs, number of recv posts: %d\n", recv_count);
            // step2:  for all directions
            //front: z- no pack. &matrix[0][0][0] - &matrix[0][15][15]
            int x,y,z;
            for(y = 0; y < arraysize; ++y){
                for(x = 0; x < arraysize; ++x){
                    front[y*arraysize+x] = matrix(1,y,x);
                }
            }
            //back: z+ no pack. &matrix[15][0][0] - &matrix[15][15][15]
            for(y = 0; y < arraysize; ++y){
                for(x = 0; x < arraysize; ++x){
                    back[y*arraysize+x] = matrix(arraysize,y,x);
                }
                }
            //left: x-
            for(z = 0; z < arraysize; ++z){
                for(y = 0; y < arraysize; ++y){
                    left[z*arraysize+y] = matrix(z,y,1);
                }
            }
            //right: x+
            for(z = 0; z < arraysize; ++z){
                for(y = 0; y < arraysize; ++y){
                    right[z*arraysize+y] = matrix(z,y,arraysize);
                }
                }
            //up: y-
            for(z = 0; z < arraysize; ++z){
                for(x = 0; x < arraysize; ++x){
                    up[z*arraysize+x] = matrix(z,1,x);
                }
            }
            //down: y+
            for(z = 0; z < arraysize; ++z){
                for(x = 0; x < arraysize; ++x){
                    down[z*arraysize+x] = matrix(z,arraysize,x);
                }
                }
                        if(DEBUG) if (rank ==0)printf("Done packing\n");

            // step 3 mpi isend to all neighbors
            //has front neighbor
            if(nb[0] != -1){
                        MPI_Isend(front, count, MPI_DOUBLE, nb[0], 2, MPI_COMM_WORLD,&request[6]);
                                send_count++;
            }
            //has back neighbor
            if(nb[1] != -1){
                MPI_Isend(back, count, MPI_DOUBLE, nb[1], 1, MPI_COMM_WORLD,&request[7]);
                                send_count++;
            }
            //has left neighbor
            if(nb[2] != -1){
                MPI_Isend(left, count, MPI_DOUBLE, nb[2], 4, MPI_COMM_WORLD,&request[8]);
                                send_count++;
            }
            //has right neighbor
            if(nb[3] != -1){
                MPI_Isend(right, count, MPI_DOUBLE, nb[3], 3, MPI_COMM_WORLD,&request[9]);
                                send_count++;
            }
            //has up neighbor
            if(nb[4] != -1){
                MPI_Isend(up, count, MPI_DOUBLE, nb[4], 6, MPI_COMM_WORLD,&request[10]);
                                send_count++;
            }
            //has down neighbor
            if(nb[5] != -1){
                MPI_Isend(down, count, MPI_DOUBLE, nb[5], 5, MPI_COMM_WORLD,&request[11]);
                                send_count++;
            }
                        if(DEBUG) if (rank ==0)printf("Done isending, number of sends post: %d\n", send_count);

            //step 4: wait all and unpack isend 6, irecv6
            for (i = 0; i < send_count + recv_count; i++){
                MPI_Waitany(12, request, &index, status);//send_count + recv_count, request, &index, status);
                                if(DEBUG) if (rank ==0)printf("pass waitany once,index is : %d\n", index);
                            if (index < 6) { //
                switch(index) {
                    case 0:   //front irecv done
                        for(y = 0; y < arraysize; ++y){
                            for(x = 0; x < arraysize; ++x){
                                matrix(0,y,x) = rfront[y*arraysize+x];
                            }
                        }
                        break;
                    case 1:   // irecv done
                        for(y = 0; y < arraysize; ++y){
                            for(x = 0; x < arraysize; ++x){
                                matrix((arraysize+1),y,x) = rback[y*arraysize+x];
                            }
                        }
                                                if(DEBUG) if (rank ==0)printf("recved from 4\n");
                        break;
                    case 2:   //front irecv done
                                                //if(DEBUG) if (rank ==0)printf("recved from 2\n");
                        for(z = 0; z < arraysize; ++z){
                            for(y = 0; y < arraysize; ++y){
                                matrix(z,y,0) = rleft[z*arraysize+y];
                            }
                        }
                        break;
                    case 3:   // irecv done
                        for(z = 0; z < arraysize; ++z){
                            for(y = 0; y < arraysize; ++y){
                                matrix(z,y,(arraysize+1)) = rright[z*arraysize+y];
                                                              //  if(DEBUG) if (rank == 0) printf("unpacked: %d\n",y*arraysize+x);
                            }
                        }
                                                if(DEBUG) if (rank ==0)printf("recved from 1\n");
                        break;
                    case 4:   //front irecv done
                        for(z = 0; z < arraysize; ++z){
                            for(x = 0; x < arraysize; ++x){
                                matrix(z,0,x) = rup[z*arraysize+x];
                            }
                        }
                        break;
                    case 5:   // irecv done
                                                if(DEBUG) if (rank ==0)printf("recved from 2\n");
                        for(z = 0; z < arraysize; ++z){
                            for(x = 0; x < arraysize; ++x){
                                matrix(z,(arraysize+1),x) = rdown[z*arraysize+x];
                            }
                        }
                        break;
                }
                            }
            }

                    tmm = MPI_Wtime();
                    t0m[size_iter] += tmm - tm;
                    //computation
            if(DEBUG) if (rank ==0)printf("start computation of manual\n");
                    for (it1 = 1; it1 < arraysize+1; it1++) {
                        for (it2 = 1; it2 < arraysize+1; it2++) {
                            for (it3 = 1; it3 < arraysize+1; it3++) {
                    //if ( arraysize == 32) if(DEBUG) if(rank==0) printf("%f\t",matrix(it1,it2,it3) );
                    //it1new = it1-1;
                                    matrix(it1,it2,it3) = (matrix(it1,it2,it3) + matrix((it1-1),it2,it3) + matrix(it1,(it2-1),it3)+ matrix(it1,it2,(it3-1))+ matrix((it1+1),it2,it3) + matrix(it1,(it2+1),it3)+ matrix(it1,it2,(it3+1)))/7;
                                    //if ( arraysize == 32) if(DEBUG) if(rank==0) printf("%d\t",matrix(it1,it2,it3) );
                            }
                        }
                    }
            if (DEBUG) if (rank==0) printf("matrix size is: %d, done 1 in while loop\n", arraysize);
            MPI_Barrier(MPI_COMM_WORLD);
            iteration--;
        }
                free(front);
                free(back);
                free(left);
                free(right);
                free(up);
                free(down);
                free(rfront);
                free(rback);
                free(rleft);
                free(rright);
                free(rup);
                free(rdown);
                te = MPI_Wtime();
                t0[size_iter] = te - ts;
        if (DEBUG) if (rank == 0) printf("DONE1, t0: %.6f\n", t0[size_iter]);
        //***********************************new algorithm*****************************

        MPI_Barrier(MPI_COMM_WORLD);
        ts = MPI_Wtime();
        //create contiguous buffer for each direction


        newfront = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (newfront == NULL) printf("front malloc failed\n");
        back2 = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (back2 == NULL) printf("back malloc failed\n");
        left2 = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (left2 == NULL) printf("left malloc failed\n");
        right2 = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (right2 == NULL) printf("front malloc failed\n");
        up2 = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (up2 == NULL) printf("front malloc failed\n");
        down2 = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (down2 == NULL) printf("front malloc failed\n");
        rnewfront = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (rnewfront == NULL) printf("front malloc failed\n");
        rback2 = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (rback2 == NULL) printf("front malloc failed\n");
        rleft2 = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (rleft2 == NULL) printf("front malloc failed\n");
        rright2 = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (rright2 == NULL) printf("front malloc failed\n");
        rup2 = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (rup2 == NULL) printf("front malloc failed\n");
        rdown2 = (double *) malloc(arraysize*arraysize*sizeof(double));
                if (rdown2 == NULL) printf("front malloc failed\n");

                iteration = IT;
                MPI_Type_size(MPI_DOUBLE, &datatype_size);
                if(DEBUG) if(rank==0) printf("double size is : %d, and MPI_DOUBLE size is : %d \n", sizeof(double), datatype_size);

        while (iteration > 0){

                    tm = MPI_Wtime();
                    recv_count=0;
                    send_count=0;
                    index = -1;
                    int req_iter;
                    for (req_iter = 0; req_iter<12; req_iter++) request[req_iter] = MPI_REQUEST_NULL;
            //step 1 irecv from neighbors
            //has front neighbor
            if(nb[0] != -1){
                        MPI_Irecv(rnewfront, count, MPI_DOUBLE, nb[0], 1, MPI_COMM_WORLD,&request[0]);
                                recv_count++;
            }
            //has back neighbor
            if(nb[1] != -1){
                MPI_Irecv(rback2, count, MPI_DOUBLE, nb[1], 2, MPI_COMM_WORLD,&request[1]);
                                recv_count++;
            }
            //has left neighbor
            if(nb[2] != -1){
                MPI_Irecv(rleft2, count, MPI_DOUBLE, nb[2], 3, MPI_COMM_WORLD,&request[2]);
                                recv_count++;
            }
            //has right neighbor
            if(nb[3] != -1){
                MPI_Irecv(rright2, count, MPI_DOUBLE, nb[3], 4, MPI_COMM_WORLD,&request[3]);
                                recv_count++;
            }
            //has up neighbor
            if(nb[4] != -1){
                MPI_Irecv(rup2, count, MPI_DOUBLE, nb[4], 5, MPI_COMM_WORLD,&request[4]);
                                recv_count++;
            }
            //has down neighbor
            if(nb[5] != -1){
                MPI_Irecv(rdown2, count, MPI_DOUBLE, nb[5], 6, MPI_COMM_WORLD,&request[5]);
                                recv_count++;
            }
                        if(DEBUG) if (rank ==0)printf("Done posting Irecvs, number of recv posts: %d\n", recv_count);
            // step2:  for all directions
            //front: z- no pack. &matrix[0][0][0] - &matrix[0][15][15]

                        int x,y,z;
            if (iteration == IT){
                for(y = 0; y < arraysize; ++y){
                    for(x = 0; x < arraysize; ++x){
                    newfront[y*arraysize+x] = matrix(1,y,x);
                }
                }
                //back: z+ no pack. &matrix[15][0][0] - &matrix[15][15][15]
                for(y = 0; y < arraysize; ++y){
                for(x = 0; x < arraysize; ++x){
                    back2[y*arraysize+x] = matrix(arraysize,y,x);
                }
                    }
                //left: x-
                for(z = 0; z < arraysize; ++z){
                    for(y = 0; y < arraysize; ++y){
                    left2[z*arraysize+y] = matrix(z,y,1);
                }
                }
                //right: x+
                for(z = 0; z < arraysize; ++z){
                for(y = 0; y < arraysize; ++y){
                    right2[z*arraysize+y] = matrix(z,y,arraysize);
                }
                    }
                //up: y-
                for(z = 0; z < arraysize; ++z){
                    for(x = 0; x < arraysize; ++x){
                    up2[z*arraysize+x] = matrix(z,1,x);
                }
                }
                //down: y+
                for(z = 0; z < arraysize; ++z){
                for(x = 0; x < arraysize; ++x){
                    down2[z*arraysize+x] = matrix(z,arraysize,x);
                }
                    }
            }
            // step 3 mpi isend to all neighbors
            //has front neighbor
            if(nb[0] != -1){
                        MPI_Isend(newfront, count, MPI_DOUBLE, nb[0], 2, MPI_COMM_WORLD,&request[6]);
                                send_count++;
            }
            //has back neighbor
            if(nb[1] != -1){
                MPI_Isend(back2, count, MPI_DOUBLE, nb[1], 1, MPI_COMM_WORLD,&request[7]);
                                send_count++;
            }
            //has left neighbor
            if(nb[2] != -1){
                MPI_Isend(left2, count, MPI_DOUBLE, nb[2], 4, MPI_COMM_WORLD,&request[8]);
                                send_count++;
            }
            //has right neighbor
            if(nb[3] != -1){
                MPI_Isend(right2, count, MPI_DOUBLE, nb[3], 3, MPI_COMM_WORLD,&request[9]);
                                send_count++;
            }
            //has up neighbor
            if(nb[4] != -1){
                MPI_Isend(up2, count, MPI_DOUBLE, nb[4], 6, MPI_COMM_WORLD,&request[10]);
                                send_count++;
            }
            //has down neighbor
            if(nb[5] != -1){
                MPI_Isend(down2, count, MPI_DOUBLE, nb[5], 5, MPI_COMM_WORLD,&request[11]);
                                send_count++;
            }
                        if(DEBUG) if (rank ==0)printf("Done isending, number of sends post: %d\n", send_count);

            //step 4: wait all and unpack isend 6, irecv6
            for (i = 0; i < send_count + recv_count; i++){
                MPI_Waitany(12, request, &index, status);//send_count + recv_count, request, &index, status);
                            if(DEBUG) if (rank ==0)printf("pass waitany once,index is : %d\n", index);
                            if (index < 6) { //
                switch(index) {
                    case 0:   //front irecv done
                        for(y = 0; y < arraysize; ++y){
                            for(x = 0; x < arraysize; ++x){
                                matrix(0,y,x) = rnewfront[y*arraysize+x];
                            }
                        }
                        break;
                    case 1:   // irecv done
                        for(y = 0; y < arraysize; ++y){
                            for(x = 0; x < arraysize; ++x){
                                matrix((arraysize+1),y,x) = rback2[y*arraysize+x];
                            }
                        }
                                                if(DEBUG) if (rank ==0)printf("recved from 4\n");
                        break;
                    case 2:   //front irecv done
                                                if(DEBUG) if (rank ==0)printf("recved from 2\n");
                        for(z = 0; z < arraysize; ++z){
                            for(y = 0; y < arraysize; ++y){
                                matrix(z,y,0) = rleft2[z*arraysize+y];
                            }
                        }
                        break;
                    case 3:   // irecv done
                        for(z = 0; z < arraysize; ++z){
                            for(y = 0; y < arraysize; ++y){
                                matrix(z,y,(arraysize+1)) = rright2[z*arraysize+y];
                                                              //  if(DEBUG) if (rank == 0) printf("unpacked: %d\n",y*arraysize+x);
                            }
                        }
                                                if(DEBUG) if (rank ==0)printf("recved from 1\n");
                        break;
                    case 4:   //front irecv done
                        for(z = 0; z < arraysize; ++z){
                            for(x = 0; x < arraysize; ++x){
                                matrix(z,0,x) = rup2[z*arraysize+x];
                            }
                        }
                        break;
                    case 5:   // irecv done
                                                if(DEBUG) if (rank ==0)printf("recved from 2\n");
                        for(z = 0; z < arraysize; ++z){
                            for(x = 0; x < arraysize; ++x){
                                matrix(z,(arraysize+1),x) = rdown2[z*arraysize+x];
                            }
                        }
                        break;
                }
                            }
            }

                    tmm = MPI_Wtime();
                    t2m[size_iter] += tmm - tm;
                    //computation
                    //int putfront, putback, putleft, putright, putup, putdown;
                    for (it1 = 1; it1 < arraysize+1; it1++) {
                        //putfront = it1 == 1;
                        //putback = it1 == arraysize;
                        if (it1 == 1){
                        for (it2 = 1; it2 < arraysize+1; it2++) {
                            for (it3 = 1; it3 < arraysize+1; it3++) {
                                    matrix(it1,it2,it3) = (matrix(it1,it2,it3) + matrix((it1-1),it2,it3) + matrix(it1,(it2-1),it3)+ matrix(it1,it2,(it3-1))+ matrix((it1+1),it2,it3) + matrix(it1,(it2+1),it3)+ matrix(it1,it2,(it3+1)))/7;
                                    //if(DEBUG) if((rank==0)&& (size_iter == 0)) printf("%d\t",matrix(it1,it2,it3) );
                            }
                        }
                            for(y = 0; y < arraysize; ++y){
                                for(x = 0; x < arraysize; ++x){
                                    newfront[y*arraysize+x] = matrix(1,y,x);
                                }
                            }
                        }
                        else if (it1 == arraysize){
                        for (it2 = 1; it2 < arraysize+1; it2++) {
                            for (it3 = 1; it3 < arraysize+1; it3++) {
                                    matrix(it1,it2,it3) = (matrix(it1,it2,it3) + matrix((it1-1),it2,it3) + matrix(it1,(it2-1),it3)+ matrix(it1,it2,(it3-1))+ matrix((it1+1),it2,it3) + matrix(it1,(it2+1),it3)+ matrix(it1,it2,(it3+1)))/7;
                                    //if(DEBUG) if((rank==0)&& (size_iter == 0)) printf("%d\t",matrix(it1,it2,it3) );
                            }
                        }
                            for(y = 0; y < arraysize; ++y){
                                for(x = 0; x < arraysize; ++x){
                                    back2[y*arraysize+x] = matrix(arraysize,y,x);
                                }
                            }
                        }
                        else {
                            for (it2 = 1; it2 < arraysize+1; it2++) {
                            for (it3 = 1; it3 < arraysize+1; it3++) {
                                    matrix(it1,it2,it3) = (matrix(it1,it2,it3) + matrix((it1-1),it2,it3) + matrix(it1,(it2-1),it3)+ matrix(it1,it2,(it3-1))+ matrix((it1+1),it2,it3) + matrix(it1,(it2+1),it3)+ matrix(it1,it2,(it3+1)))/7;
                                    //if(DEBUG) if((rank==0)&& (size_iter == 0)) printf("%d\t",matrix(it1,it2,it3) );
                            }
                        }
                        }



                    }

                //left: x-
                for(z = 0; z < arraysize; ++z){
                    for(y = 0; y < arraysize; ++y){
                    left2[z*arraysize+y] = matrix(z,y,1);
                }
                }
                //right: x+
                for(z = 0; z < arraysize; ++z){
                for(y = 0; y < arraysize; ++y){
                    right2[z*arraysize+y] = matrix(z,y,arraysize);
                }
                    }
                //up: y-
                for(z = 0; z < arraysize; ++z){
                    for(x = 0; x < arraysize; ++x){
                    up2[z*arraysize+x] = matrix(z,1,x);
                }
                }
                //down: y+
                for(z = 0; z < arraysize; ++z){
                for(x = 0; x < arraysize; ++x){
                    down2[z*arraysize+x] = matrix(z,arraysize,x);
                }
                }


            if (DEBUG) if (rank==0) printf("matrix size is: %d, done packing &&  in while loop\n", arraysize);
            MPI_Barrier(MPI_COMM_WORLD);
            iteration--;
        }
        if (DEBUG) if (rank==0) printf("freeing ~\n");
                free(newfront);
        if (DEBUG) if (rank==0) printf("freeing2 ~\n");
                free(back2);
                free(left2);
                free(right2);
                free(up2);
                free(down2);
                free(rnewfront);
                free(rback2);
                free(rleft2);
                free(rright2);
                free(rup2);
                free(rdown2);


                te = MPI_Wtime();
                t2[size_iter] = te - ts;
        if (DEBUG) if (rank == 0) printf("DONE3, t2: %.6f\n", t2[size_iter]);


        //*************************************vector type*********************************************
        MPI_Barrier(MPI_COMM_WORLD);
        ts = MPI_Wtime();
                MPI_Type_contiguous(arraysize, MPI_DOUBLE, &type_row);
                MPI_Type_commit(&type_row);
                MPI_Type_vector(arraysize, 1, arraysize, type_row,  &type_ud);
                MPI_Type_commit(&type_ud);  //type of up and down planes
                MPI_Type_vector(count, 1, arraysize, MPI_DOUBLE,  &type_lr);
                MPI_Type_commit(&type_lr);  //type of left and right planes
                MPI_Type_size(type_lr, &datatype_size);
                if(DEBUG) if(rank==0) printf("type_lr size is : %d \n", datatype_size);
                iteration = IT;


        while (iteration > 0){

                    tm = MPI_Wtime();
                    recv_count=0;
                    send_count=0;
                    index = -1;
                    int req_iter;
                    for (req_iter = 0; req_iter<12; req_iter++) request[req_iter] = MPI_REQUEST_NULL;
            //step 1 irecv from neighbors
            //has front neighbor
            if(nb[0] != -1){
                        MPI_Irecv(&matrix(0,1,1), count, MPI_DOUBLE, nb[0], 1, MPI_COMM_WORLD,&request[0]);
                                recv_count++;
            }
            //has back neighbor
            if(nb[1] != -1){
                MPI_Irecv(&matrix((arraysize+1),1,1), count, MPI_DOUBLE, nb[1], 2, MPI_COMM_WORLD,&request[1]);
                                recv_count++;
            }
            //has left neighbor
            if(nb[2] != -1){
                MPI_Irecv(&matrix(1,1,0), 1, type_lr, nb[2], 3, MPI_COMM_WORLD,&request[2]);
                                recv_count++;
            }
            //has right neighbor
            if(nb[3] != -1){
                MPI_Irecv(&matrix(1,1,(arraysize+1)), 1, type_lr, nb[3], 4, MPI_COMM_WORLD,&request[3]);
                                recv_count++;
            }
            //has up neighbor
            if(nb[4] != -1){
                MPI_Irecv(&matrix(1,0,1), 1, type_ud, nb[4], 5, MPI_COMM_WORLD,&request[4]);
                                recv_count++;
            }
            //has down neighbor
            if(nb[5] != -1){
                MPI_Irecv(&matrix(1,(arraysize+1),1), 1, type_ud, nb[5], 6, MPI_COMM_WORLD,&request[5]);
                                recv_count++;
            }
                        if(DEBUG) if (rank ==0)printf("Done posting Irecvs, number of recv posts: %d\n", recv_count);
            // step2:  no step 2 no packing is required


            // step 3 mpi isend to all neighbors
            //has front neighbor
            if(nb[0] != -1){
                        MPI_Isend(&matrix(1,1,1), count, MPI_DOUBLE, nb[0], 2, MPI_COMM_WORLD,&request[6]);
                                send_count++;
            }
                        if(DEBUG) if (rank ==0)printf("Done isend to front neighbor\n");

            //has back neighbor
            if(nb[1] != -1){
                MPI_Isend(&matrix(arraysize,1,1), count, MPI_DOUBLE, nb[1], 1, MPI_COMM_WORLD,&request[7]);
                                send_count++;
            }
                        if(DEBUG) if (rank ==0)printf("Done isend to back neighbor\n");
            //has left neighbor
            if(nb[2] != -1){
                MPI_Isend(&matrix(1,1,1), 1, type_lr, nb[2], 4, MPI_COMM_WORLD,&request[8]);
                                send_count++;
            }
                        if(DEBUG) if (rank ==0)printf("Done isend to left neighbor\n");
            //has right neighbor
            if(nb[3] != -1){
                                if(DEBUG) if(rank==0) printf("matrix starting point is %.6f\n",matrix[1,1,arraysize]);
                MPI_Isend(&matrix(1,1,arraysize), 1, type_lr, nb[3], 3, MPI_COMM_WORLD,&request[9]);
                                send_count++;
            }
                        if(DEBUG) if (rank ==0)printf("Done isend to right neighbor\n");
            //has up neighbor
            if(nb[4] != -1){
                MPI_Isend(&matrix(1,1,1), 1, type_ud, nb[4], 6, MPI_COMM_WORLD,&request[10]);
                                send_count++;
            }
                        if(DEBUG) if (rank ==0)printf("Done isend to up neighbor\n");
            //has down neighbor
            if(nb[5] != -1){
                MPI_Isend(&matrix(1,arraysize,1), 1, type_ud, nb[5], 5, MPI_COMM_WORLD,&request[11]);
                                send_count++;
            }
                        if(DEBUG) if (rank ==0)printf("Done isending, number of sends post: %d\n", send_count);

            //step 4: wait all and unpack isend 6, irecv6
            for (i = 0; i < send_count + recv_count; i++){
                MPI_Waitany(12, request, &index, status);//send_count + recv_count, request, &index, status);
                                if(DEBUG) if (rank ==0)printf("pass waitany once,index is : %d\n", index);

                        }
            tmm = MPI_Wtime();
            t1m[size_iter] += (tmm - tm);

            if (DEBUG) if (rank==0) printf("matrix size is: %d, done 1 in while loop\n", arraysize);
            iteration--;
                    //computation
                    for (it1 = 1; it1 < arraysize+1; it1++) {
                        for (it2 = 1; it2 < arraysize+1; it2++) {
                            for (it3 = 1; it3 < arraysize+1; it3++) {
                                    matrix(it1,it2,it3) = (matrix(it1,it2,it3) + matrix((it1-1),it2,it3) + matrix(it1,(it2-1),it3)+ matrix(it1,it2,(it3-1))+ matrix((it1+1),it2,it3) + matrix(it1,(it2+1),it3)+ matrix(it1,it2,(it3+1)))/7;
                                    //if(DEBUG) if((rank==0)&& (size_iter == 0)) printf("%d\t",matrix(it1,it2,it3) );
                            }
                        }
                    }
            MPI_Barrier(MPI_COMM_WORLD);


        }

                MPI_Type_free(&type_row);
                MPI_Type_free(&type_ud);
                MPI_Type_free(&type_lr);


                te = MPI_Wtime();
                t1[size_iter] = te - ts;
                free(matrix);
    }

    if (rank == 0)
    {
        printf("#STENCIL EDGE EXCHANGE, MEASURE TIME OF EXCHANGE(seconds)\n# --> ACTUAL {IRECVs,PACKINGs, ISENDs, WAITANY, UNPACKINGs} TIME\n");
        printf("MPI IMPLEMENTATION, NON/BLK, ITERATION,SIZE,DATA_TYPE,TIME(ms) PER TIME STEP\n");
        for (size_iter = 0; size_iter < STEP; size_iter=size_iter+1){
            printf(MPI_IMP",NONBLK, %d,%d,MANUALLY COPY,%.6f\n", IT,ARRAY_SIZE*(size_iter+1), t0m[size_iter]);
            printf(MPI_IMP",NONBLK, %d,%d,VECTOR,%.6f\n", IT,ARRAY_SIZE*(size_iter+1), t1m[size_iter]);
            //printf(MPI_IMP",NONBLK, %d,%d,DM TIME,%.6f\n", IT,ARRAY_SIZE*(size_iter+1), t2m[size_iter]);
            printf(MPI_IMP",NONBLK, %d,%d,MANUALLY COPY TOTAL TIME,%.6f\n", IT,ARRAY_SIZE*(size_iter+1), t0[size_iter]);
            printf(MPI_IMP",NONBLK, %d,%d,VECTOR TOTAL TIME,%.6f\n", IT,ARRAY_SIZE*(size_iter+1), t1[size_iter]);
            //printf(MPI_IMP",NONBLK, %d,%d,DM TOTAL TIME,%.6f\n", IT,ARRAY_SIZE*(size_iter+1), t2[size_iter]);
        }
        fflush(stdout);
    }
    MPI_Finalize();
    return 0;;
}

