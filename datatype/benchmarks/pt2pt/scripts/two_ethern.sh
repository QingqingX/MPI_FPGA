#!/bin/bash -l
#$ -pe mpi_12_tasks_per_node 24
cat $PE_HOSTFILE | cut -d '.' -f 1 | sed 's/$/:1/g' > hosts
echo '#One RANK PER NODE'
module load mpich
echo '#MPICH'
/share/pkg/mpich/3.2/install/bin/mpiexec -n 2 -f hosts ./new_datatype_mpich
echo '#Open MPI'
module rm mpich
/usr/local/bin/mpirun -mca btl tcp,sm,self -np 2 -npernode 1 ./new_datatype_ompi
echo '#MVAPICH'
MPICH_NEMESIS_NETMOD=tcp /usr3/graduate/qx/tcp_mvapich2_install/bin/mpirun -n 2 -f hosts ./new_datatype_mvapich2


