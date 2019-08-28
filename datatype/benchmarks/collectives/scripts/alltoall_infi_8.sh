#!/bin/bash -l
#$ -pe mpi_28_tasks_per_node 224
cat $PE_HOSTFILE | cut -d '.' -f 1 | sed 's/$/:28/g' > hosts_alltoall_infi_8
echo '#ALLTOALL'
echo '#28 RANKS per NODE 112 IN TOTAL INFI'
module load gcc/7.2.0

echo '#MVAPICH2'
module load mvapich2/2.3_gcc-7.2.0
echo "#$(which mpiexec)"
#MV2_ENABLE_AFFINITY=0
mpiexec -n 224 -bind-to hwthread -f hosts_alltoall_infi_8 ./alltoall_mvapich2

echo '#Open MPI'
#module rm gcc/7.2.0
module load openmpi/3.1.2_gcc-7.2.0
echo "#$(which mpirun)"
/share/pkg/openmpi/3.1.2_gcc-7.2.0/install/bin/mpirun -np 224 -npernode 28 ./alltoall_ompi

#/usr3/graduate/qx/mvapich2_install/bin/mpirun -n 2 -f hosts ./new_datatype_mvapich2

