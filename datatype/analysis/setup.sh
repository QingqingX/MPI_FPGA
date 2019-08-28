module load gcc/8.1.0

module load papi/2018-01
module load openmpi/3.1.1
echo '#Open MPI'
mpirun -np 2 -npernode 2 ./vector_sn > result_omp.txt
echo '#MPICH'
/usr3/graduate/qx/mpich-3.2.1/install/bin/mpiexec -n 2 ./vector_sn_mpich > result_mpich.txt
