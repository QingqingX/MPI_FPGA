# MPI_FPGA
BU &amp; Auburn

This folder's work is published in "MPI Derived Datatypes: Performance and Portability Issues" and Qingqing Xiong's disseration. 


directories:
  analysis    --  when comparing with hardware design; we analyzed and created the tests systematically 
  benchmarks  --  For studying MPI derived datatypes' performance portability 
    application    
             matrix*    --  matrix edge transfer in minighost with various datatypes
             stencil    --  full stencil with datatype and without; this includes the exploration of a new idea: pack close to cache
    datatype_sr    --  blocking point-2-point operations with various datatypes 
    datatype_coll  --  collective operations with various datatypes
    
