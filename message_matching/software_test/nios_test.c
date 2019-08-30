/*
 * * "nios ii Message matching" tests.
 * QX, CAAD
 *
 */

#include <stdio.h>
#include "sys/alt_stdio.h"
#include <io.h>
#include <system.h>
#include <string.h>
#include <stdlib.h>
#include <altera_avalon_performance_counter.h>
#include "system.h"
#include <alt_types.h>
#define MEM_SIZE   1000   //word number
#define SEND_BASE  0x00000000
#define ITERATION 100
#define MEM_TEST 0
#define BIG_TEST 1
#define PAIRED   0
#define DATA_TYPE      1
#define SW		 0
#define PRQ      0
#define DBG      1
#define LOOP	 0


#define WAITALL 0xff111111
//PERF_BEGIN (PERFORMANCE_COUNTER_0_BASE, 3);

void pack(char * in, int blk_len, int count, int stride, char* buffer){
	int i,j;
	char * temp = buffer;
	char * in_tmp = in;
	for(i=0; i < count; i++){
		for (j=0; j < blk_len; j++){
			*temp = *in_tmp;
			temp ++;
			in_tmp++;
		}
		in_tmp = in_tmp + stride - blk_len;
	}

	buffer = temp;

}
int main()
{
  /*****initialize part of the mem*************/
  /******mystery: for loop doesn't work*********/
  printf("Hello from Nios II \n");
#if MEM_TEST
  unsigned int pattern = 1;
  unsigned int offset = 0;
  //for (pattern = 1, offset = 0; offset < MEM_SIZE; pattern++, offset+=4)

      IOWR_32DIRECT(SEND_BASE, offset, pattern);

      printf(".");
      offset = offset + 4;
      pattern++;
      IOWR_32DIRECT(SEND_BASE, offset, pattern);
      printf(".");
      offset = offset + 4;
	  pattern++;
      IOWR_32DIRECT(SEND_BASE, offset, pattern);
      printf(".");
      offset = offset + 4;
      pattern++;
      IOWR_32DIRECT(SEND_BASE, offset, pattern);
      printf(".");
      offset = offset + 4;
      pattern++;
      IOWR_32DIRECT(SEND_BASE, offset, pattern);
      printf(".");

  printf("Finishes initializing memory!!! \n\n\n");
#endif
  /******test send and recv of short messages************/
#if (BIG_TEST==0)
  int it;
  for (it = 0; it < ITERATION; ++it) printf("~");
  printf("Step1:   Test short send and recv \n");
  int count = 0;
  int pass_count = 0;
  int send_data = 0x00000001;
  int send = ALT_CI_ACC_0(7, 0x55550001, send_data);
  printf("Just sent! %08x\n", send_data);
  int recv = ALT_CI_ACC_RECV_0(7, 0x55550001, 0x00002001);
  printf("received %08x\n", recv);
  count++;
  if (send_data == recv) pass_count++;
  send_data = 0xf0000001;
  send = ALT_CI_ACC_0(7, 0x55550001, send_data);
  printf("Just sent! %08x\n", send_data);
  recv = ALT_CI_ACC_RECV_0(7, 0x55550001, 0x00002001);
  printf("received %08x\n", recv);

  count++;
  if (send_data == recv) pass_count++;
  send_data = 0xa0e00001;
  send = ALT_CI_ACC_0(7, 0x55550001, send_data);
  printf("Just sent! %08x\n", send_data);
  recv = ALT_CI_ACC_RECV_0(7, 0x55550001, 0x00002001);
  printf("received %08x\n", recv);
  count++;
  if (send_data == recv) pass_count++;

  if (pass_count == count)
  {
	  printf("Pass all the short send and recv tests!!!\n");
  }
  else
  {
	  printf("Test %d cases, passed %d cases......\n", count, pass_count);
  }
#endif
  /*****************test massive amount of short send and recvs*************/
#if BIG_TEST
  int k = 0x00010002;
  int pass_count = 0;
  int send_data = 0X00000003;
  unsigned int it;
  int send, irecv;
  PERF_RESET (PERFORMANCE_COUNTER_0_BASE);
  PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);

#if LOOP
    for (it = 0; it <ITERATION; ++it){
          		send_data = send_data + it  ;
          		  //if (irecv == send_data) pass_count++;
          	}
    PERF_END (PERFORMANCE_COUNTER_0_BASE, 1);
    printf("send_data: %d", send_data);
    perf_print_formatted_report  ((void *)PERFORMANCE_COUNTER_0_BASE,   /* defined in "system.h" */
                                   ALT_CPU_FREQ,                       /* defined in "system.h" */
                                          1,                                  /* How many sections to print */
                              ITERATION
                                        );

#endif

#if PAIRED
  for (it = 0; it <ITERATION; ++it){



	  send_data = send_data + 1;
	  	  //when mpi_type 'h01, eager short. datab is the data
	  	  send = ALT_CI_ACC_0(7, k+it, send_data);
	  	  printf("Just sent! %08x\n", send_data);
	  	irecv = ALT_CI_ACC_RECV_0(7, k+it, 0x00002003);
	  		  printf("received %08x\n", irecv);
	  if (irecv == send_data) pass_count++;
  }
  PERF_END (PERFORMANCE_COUNTER_0_BASE, 1);
  irecv = ALT_CI_ACC_RECV_0(7, 0xff111111, ITERATION);

  printf("received %08x\n", irecv);


  if (pass_count == ITERATION)
    {
  	  printf("Pass all the massive short send and recv tests!!!\n");
    }
    else
    {
  	  printf("Test %d cases, passed %d cases......\n",  ITERATION, pass_count);
    }
  if (irecv == 0x52052020) printf("waitall succeed!");
  perf_print_formatted_report  ((void *)PERFORMANCE_COUNTER_0_BASE,   /* defined in "system.h" */
                             ALT_CPU_FREQ,                       /* defined in "system.h" */
                                    1,                                  /* How many sections to print */
                        ITERATION
                                  );
#endif
#if SW

  	int send1,send2;
	char send_array[4000];
	char send_array2[400];
	int i = 0;
	for (i=0; i<4000; ++i){
	  send_array[i] = i+1;
	}
	for (i=0; i<400; ++i){
		  send_array2[i] = i*2+1;
	  }
	char buffer[2000];

	PERF_BEGIN (PERFORMANCE_COUNTER_0_BASE, 1);

	pack(send_array, 1, 200, 4, buffer);


/*
  for (it = 0; it <ITERATION; ++it){
  	  //send_data = send_data + 1;
  	  //when mpi_type 'h07, eager short. datab is the data
  	  send = ALT_CI_ACC_0(0x2, 0xa, send_data);
  #if DBG
      //printf("Just sent! %08x\n", send);
  #endif
    }


*/
	PERF_END (PERFORMANCE_COUNTER_0_BASE, 1);


    //  printf("received %08x\n", irecv);
	//if (irecv == 0x52052020) printf("waitall succeed!");
	perf_print_formatted_report  ((void *)PERFORMANCE_COUNTER_0_BASE,
							   ALT_CPU_FREQ,
									  1,
						  ITERATION
									);



#endif

#if DATA_TYPE
   k = 0;
   int send1,send2;
  int send_array[4];
  int send_array2[4];
  int i = 0;
  for (i=0; i<4; ++i){
	  send_array[i] = i+1;
  }
  for (i=0; i<4; ++i){
  	  send_array2[i] = i*2+1;
    }
  PERF_BEGIN (PERFORMANCE_COUNTER_0_BASE, 1);
  for (it = 0; it <ITERATION; ++it){
	  //send_data = send_data + 1;
	  //when mpi_type 'h02, eager short. datab is the data
	  //send1 = ALT_CI_ACC_0(0x3, 0x12341002/*dataa[11:0] is te size*/, &send_array[0]);
	  send1 = ALT_CI_ACC_0(0x0, 0x12341002/*dataa[11:0] is te size*/, 11);

	  //send2 = ALT_CI_ACC_0(0x3, 0x12341002/*dataa[11:0] is te size*/, &send_array[2]);

#if DBG
	  printf("Send array base address: %08x, %08x, %08x, %08x, Just sent! %d\n", &send_array[0], &send_array[1], &send_array[2], &send_array[3], send1);
	  //printf("Send array2 base address: %08x, %08x, %08x, %08x, Just sent! %08x\n\n", &send_array2[0], &send_array2[1], &send_array2[2], &send_array2[3], send2);
#endif
  }
/*
  int recv_array[4];

  for (it = 0; it <ITERATION; ++it){
     		  irecv = ALT_CI_ACC_RECV_0(0x2, 0x12341004,&recv_array[0]);
#if DBG
     		  printf("received %08x\n", irecv);
#endif
     		  //if (irecv == send_data) pass_count++;
    }
    irecv = ALT_CI_ACC_RECV_0(7, WAITALL, ITERATION);
*/
    PERF_END (PERFORMANCE_COUNTER_0_BASE, 1);


  //  printf("received %08x\n", irecv);
    //if (irecv == 0x52052020) printf("waitall succeed!");
    perf_print_formatted_report  ((void *)PERFORMANCE_COUNTER_0_BASE,   /* defined in "system.h" */
                               ALT_CPU_FREQ,                       /* defined in "system.h" */
                                      1,                                  /* How many sections to print */
                          ITERATION
                                    );


#endif

  /**************************PRQ****************************/
#if PRQ
   k = 0;


  int mem=0;
  for (it = 0; it <ITERATION; ++it){
     		  irecv = MPI_Irecv(7, k+it,mem);
#if DBG
     		  printf("received %08x\n", irecv);
#endif
     	}
  PERF_BEGIN (PERFORMANCE_COUNTER_0_BASE, 1);

  for (it = 0; it <ITERATION; ++it){
  	  //send_data = send_data + 1;
  	  //when mpi_type 'h07, eager short. datab is the data
  	  send = MPI_Send(7, k+it, send_data);
  #if DBG
  	  printf("Just sent! %08x\n", send);
  #endif
    }

  	//a blocking function call MPI_Waitall()
    irecv = MPI_Irecv(7, WAITALL, ITERATION);
    if (irecv == 0x52052020) printf("waitall succeed FOR ONE !");
    /*for (it = 0; it <ITERATION; ++it){
       		  irecv = MPI_Irecv(7, k+it,mem);
  #if DBG
       		  printf("received %08x\n", irecv);
  #endif
       		  //if (irecv == send_data) pass_count++;
       	}
	*/
    for (it = 0; it <ITERATION; ++it){

    	  //send_data = send_data + 1;

    	  //when mpi_type 'h01, eager short. datab is the data
    	  send = MPI_Send(7, k+it, send_data);

    #if DBG
    	  printf("Just sent! %08x\n", send);
    #endif
      }
    irecv = MPI_Irecv(7, WAITALL, ITERATION);
    for (it = 0; it <ITERATION; ++it){
       		  irecv = MPI_Irecv(7, k+it,mem);
  #if DBG
       		  printf("received %08x\n", irecv);
  #endif
       		  //if (irecv == send_data) pass_count++;
    }
/*
     for (it = 0; it <ITERATION; ++it){
    	  //send_data = send_data + 1;
    	  //when mpi_type 'h01, eager short. datab is the data
    	  send = MPI_Send(7, k+it, send_data);

    #if DBG
    	  printf("Just sent! %08x\n", send);
    #endif
     }
*/
      irecv = MPI_Irecv(7, WAITALL, ITERATION);
      /*
      for (it = 0; it <ITERATION; ++it){
         		  irecv = MPI_Irecv(7, k+it,mem);
    #if DBG
         		  printf("received %08x\n", irecv);
    #endif
         		  //if (irecv == send_data) pass_count++;
      }

      for (it = 0; it <ITERATION; ++it){
     	  //send_data = send_data + 1;
      	  //when mpi_type 'h01, eager short. datab is the data
      	  send = MPI_Send(7, k+it, send_data);

      #if DBG
      	  printf("Just sent! %08x\n", send);
      #endif
      }
	*/
    PERF_END (PERFORMANCE_COUNTER_0_BASE, 1);
    //    irecv = MPI_Irecv(7, WAITALL, ITERATION);
    if (irecv == 0x52052020) printf("waitall succeed FOR TWO!");
    perf_print_formatted_report  ((void *)PERFORMANCE_COUNTER_0_BASE,   /* defined in "system.h" */
                               ALT_CPU_FREQ,                       /* defined in "system.h" */
                                      1,                                  /* How many sections to print */
                          ITERATION
                                    );


#endif
#endif
  /******** test the initialized memory*********************/
#if MEM_TEST
  offset = 0;
  pattern = 1;
  int mem_count = 0;
  if ( IORD_32DIRECT(SEND_BASE, offset) != pattern ) mem_count++;
  offset = offset + 4;
  pattern++;
  if ( IORD_32DIRECT(SEND_BASE, offset) != pattern ) mem_count++;
  offset = offset + 4;
  pattern++;
  if ( IORD_32DIRECT(SEND_BASE, offset) != pattern ) mem_count++;
  offset = offset + 4;
  pattern++;
  if ( IORD_32DIRECT(SEND_BASE, offset) != pattern ) mem_count++;
  offset = offset + 4;
  pattern++;
  if ( IORD_32DIRECT(SEND_BASE, offset) != pattern ) mem_count++;

  if (mem_count == 0){
	  printf("\n\nPassed memory read test!!!\n");
  }
  else {
	  printf("\n\nFailed memory read test:   %d failed\n\n", mem_count);
  }
#endif
  /********test send and recv long messages*****************/
/*
  send = ALT_CI_ACC_0(7, 0x55550001, 0x00000003);
  printf("Just sent! %08x \n", send);
*/
  //recv = ALT_CI_ACC_RECV_0(7, 0x55550001, 0x00002002);
  //printf("received %08x\n", recv);

  return 0;
}
