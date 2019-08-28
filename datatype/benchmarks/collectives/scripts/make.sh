#!/bin/bash -l

#module load mpich
#mpicc -DMPI_IMP='"MPICH"' -L/share/pkg/papi/2018-01/install/lib -lpapi -I/share/pkg/papi/2018-01/install/include datatype_sr.c -o datatype_sr_mpich

#module rm mpich
#mpicc -DMPI_IMP='"OPEN MPI"' -L/share/pkg/papi/2018-01/install/lib -lpapi -I/share/pkg/papi/2018-01/install/include datatype_sr.c -o datatype_sr_ompi
module load gcc/7.2.0
#module rm gcc/7.2.0
module load mpich
#/usr3/graduate/qx/mpich-3.2.1/install/bin/mpicc -DMPI_IMP='"MPICH"' datatype_sr.c -o datatype_sr_mympich
/share/pkg/mpich/3.2/install/bin/mpicc -DMPI_IMP='"MPICH"' -O3 new_datatype.c -o new_datatype_mpich
echo 'done build mpich'


#module load openmpi/3.1.2_gcc-7.2.0
module rm mpich
/share/pkg/openmpi/3.1.2_gcc-7.2.0/install/bin/mpicc -DMPI_IMP='"OPEN MPI"' -O3 new_datatype.c -o new_datatype_ompi
echo 'done build ompi'

/usr3/graduate/qx/mvapich2_install/bin/mpicc -DMPI_IMP='"MVAPICH2"' -O3 new_datatype.c -o new_datatype_mvapich2
echo 'done build mvapich2'
