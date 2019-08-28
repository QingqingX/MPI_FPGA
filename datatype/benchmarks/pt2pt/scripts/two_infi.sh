#!/bin/bash -l
#$ -pe mpi_12_tasks_per_node 24
cat $PE_HOSTFILE | cut -d '.' -f 1 | sed 's/$/:1/g' > hosts
echo '#One RANK PER NODE'
echo '#Open MPI'
module rm mpich
/usr/local/bin/mpirun -np 2 -npernode 1 ./new_datatype_ompi
echo '#MVAPICH'
/usr3/graduate/qx/mvapich2_install/bin/mpirun -n 2 -f hosts ./new_datatype_mvapich2

