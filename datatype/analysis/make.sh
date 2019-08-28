#!/bin/bash -l

#module load mpich
#mpicc -DMPI_IMP='"MPICH"' -L/share/pkg/papi/2018-01/install/lib -lpapi -I/share/pkg/papi/2018-01/install/include datatype_sr.c -o datatype_sr_mpich

#module rm mpich
#mpicc -DMPI_IMP='"OPEN MPI"' -L/share/pkg/papi/2018-01/install/lib -lpapi -I/share/pkg/papi/2018-01/install/include datatype_sr.c -o datatype_sr_ompi
module load gcc/8.1.0
#module load mpich
#/usr3/graduate/qx/mpich-3.2.1/install/bin/mpicc -DMPI_IMP='"MPICH"' datatype_sr.c -o datatype_sr_mympich

#mpicc -DMPI_IMP='"MPICH"' new_datatype.c -o new_datatype_mpich

#module rm mpich
module load openmpi/3.1.1
#which mpicc
mpicc -O3 -DMPI_IMP='"OPEN MPI"' -L/share/pkg/papi/2018-01/install/lib -lpapi -I/share/pkg/papi/2018-01/install/include vector_sn.c -o vector_sn
#/share/pkg/openmpi/3.1.2_gcc-7.2.0/install/bin/mpicc -O3 -DMPI_IMP='"OPEN MPI"' new_datatype.c -o new_datatype_ompi
#module rm openmpi/3.1.2_gcc-7.2.0
#module load mvapich2/2.3_gcc-7.2.0
#which mpicc
#mpicc -O3 -DMPI_IMP='"MVAPICH2"' new_datatype.c -o new_datatype_mvapich2
