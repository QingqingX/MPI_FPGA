
This folder's work is published in "MPI Derived Datatypes: Performance and Portability Issues" and Qingqing Xiong's disseration. 


directories:
*  analysis    --  when comparing with hardware design; we analyzed and created the tests systematically 
*  benchmarks  --  For studying MPI derived datatypes' performance portability 
>  *    application    
>  *    datatype_sr    --  blocking point-2-point operations with various datatypes 
>  *    datatype_coll  --  collective operations with various datatypes


The performance of the same derived datatype might not be portable to different MPI implementations, or between different pairs of
processes in a single implementation, depending on system topology and data-transfer paths. Another issue is with derived datatype normalization: the same layout represented with different derived datatypes may or may not have the same performance.
These hidden performance behaviors might affect the overall performance of an application greatly, especially when moving code from one implementation to another, when the programmer might need to recode to get performance improvement out of the optimized code. That obstacle can lead application programmers to do all their own data marshaling, which, in high-quality implementations and where persistent operations are used, could be done more efficiently by the implementation.

We provide benchmarks and scripts to study the performance portability issues.

