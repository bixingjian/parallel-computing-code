#!/bin/bash
#SBATCH --job-name=dy      	## Name of the job.
#SBATCH --output=mandelbrot_cyclic.out		## Output log file
#SBATCH --error=mandelbrot_cyclic.err		## Error log file
#SBATCH -A class-eecs120     		## Account to charge
#SBATCH -p standard          		## Partition/queue name
#SBATCH --nodes=4     		## Number of nodes
#SBATCH --ntasks=40  			## Number of tasks (MPI processes)

# Module load boost
module load boost/1.78.0/gcc.11.2.0

# Module load MPI
module load openmpi/4.1.2/intel.2022.2


# make clean

# make mandelbrot_cyclic
mpirun -np $SLURM_NTASKS ./mandelbrot_cyclic 1000 1000
# mpirun -np $SLURM_NTASKS ./mandelbrot_cyclic 5000 5000

# mpirun -np $SLURM_NTASKS ./mandelbrot_block 1000 1000
# mpirun -np $SLURM_NTASKS ./mandelbrot_block 5000 5000
# mpirun -np $SLURM_NTASKS ./mandelbrot_dynamic 1000 1000
# mpirun -np $SLURM_NTASKS ./mandelbrot_dynamic 5000 5000


