#!/bin/bash -l
#$ -pe mpi_28_tasks_per_node 28
echo '#FTP TWO RANKS PER NODE'
module load gcc/8.1.0

#module load mpich
#echo '#MPICH'
#/share/pkg/mpich/3.2/install/bin/mpiexec -n 2 ./new_datatype_mpich
#/usr3/graduate/qx/mpich-3.2.1/install/bin/mpiexec -n 2 -f hosts ./datatype_mympich
module load papi/2018-01
module load openmpi/3.1.1
echo '#Open MPI'
#module rm mpich
mpirun -np 2 -npernode 2 ./new_datatype_ompi
#/usr/local/bin/mpirun -mca btl tcp,sm,self -np 2 -npernode 1 ./datatype_sr_ompi
#echo '#MVAPICH'
#module rm openmpi/3.1.2_gcc-7.2.0
#module load mvapich2/2.3_gcc-7.2.0

#mpirun -n 2 ./new_datatype_mvapich2

