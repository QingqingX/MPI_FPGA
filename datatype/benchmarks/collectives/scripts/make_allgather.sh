#!/bin/bash -l

#module load mpich
#mpicc -DMPI_IMP='"MPICH"' -L/share/pkg/papi/2018-01/install/lib -lpapi -I/share/pkg/papi/2018-01/install/include datatype_sr.c -o datatype_sr_mpich

#module rm mpich
#mpicc -DMPI_IMP='"OPEN MPI"' -L/share/pkg/papi/2018-01/install/lib -lpapi -I/share/pkg/papi/2018-01/install/include datatype_sr.c -o datatype_sr_ompi
module load gcc/7.2.0
module load mvapich2/2.3_gcc-7.2.0

which mpirun
#/usr3/graduate/qx/mpich-3.2.1/install/bin/mpicc -DMPI_IMP='"MPICH"' datatype_sr.c -o datatype_sr_mympich
mpicc -DMPI_IMP='"MVAPICH2"' -O3 allgather.c -o allgather_mvapich2
echo 'done build mvapich2'


module rm mvapich2/2.3
module load openmpi/3.1.2_gcc-7.2.0
which mpirun
/share/pkg/openmpi/3.1.2_gcc-7.2.0/install/bin/mpicc -DMPI_IMP='"OPEN MPI"' -O3 allgather.c -o allgather_ompi
echo 'done build ompi'

