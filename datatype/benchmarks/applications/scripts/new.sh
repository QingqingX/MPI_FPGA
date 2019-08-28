#!/bin/bash -l
#$ -pe mpi_12_tasks_per_node 12
cat $PE_HOSTFILE | cut -d '.' -f 1 | sed 's/$/:1/g' > hosts
echo '#two RANKs PER NODE'
module load mpich
echo '#MPICH'
/share/pkg/mpich/3.2/install/bin/mpiexec -n 2 ./test_mpich
#/usr3/graduate/qx/mpich-3.2.1/install/bin/mpiexec -n 2 -f hosts ./datatype_mympich
echo '#Open MPI'
module rm mpich
/usr/local/bin/mpirun -np 2 -npernode 2 ./test_ompi
#/usr/local/bin/mpirun -mca btl tcp,sm,self -np 2 -npernode 1 ./datatype_sr_ompi
echo '#MVAPICH'
/usr3/graduate/qx/mvapich2_install/bin/mpirun -n 2 ./test_mvapich2

