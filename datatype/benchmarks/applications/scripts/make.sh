#!/bin/bash -l

#module load mpich
#mpicc -DMPI_IMP='"MPICH"' -L/share/pkg/papi/2018-01/install/lib -lpapi -I/share/pkg/papi/2018-01/install/include datatype_sr.c -o datatype_sr_mpich

#module rm mpich
#mpicc -DMPI_IMP='"OPEN MPI"' -L/share/pkg/papi/2018-01/install/lib -lpapi -I/share/pkg/papi/2018-01/install/include datatype_sr.c -o datatype_sr_ompi

module load mpich
#/usr3/graduate/qx/mpich-3.2.1/install/bin/mpicc -DMPI_IMP='"MPICH"' datatype_sr.c -o datatype_sr_mympich
mpicc -DMPI_IMP='"MPICH"' matrix_xfer.c -o test_mpich

module rm mpich
mpicc -DMPI_IMP='"OPEN MPI"' matrix_xfer.c -o test_ompi

/usr3/graduate/qx/mvapich2_install/bin/mpicc -DMPI_IMP='"MVAPICH2"' matrix_xfer.c -o test_mvapich2
