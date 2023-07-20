#include <stdlib.h>
#include <stdio.h>

#include "cuda_utils.h"
#include "timer.c"

#define TILE_DIM 32
#define BLOCK_ROWS 8

typedef float dtype;

__global__ 
void matTrans(dtype* AT, dtype* A, int N)  {
	/* Fill your code here */
	__shared__ dtype tile[TILE_DIM][TILE_DIM+1];

    int x = blockIdx.x * TILE_DIM + threadIdx.x;
    int y = blockIdx.y * TILE_DIM + threadIdx.y;

    for (int j = 0; j < TILE_DIM; j += BLOCK_ROWS)
        tile[threadIdx.y+j][threadIdx.x] = A[(y+j)*N + x];

    __syncthreads();

    x = blockIdx.y * TILE_DIM + threadIdx.x;  
    y = blockIdx.x * TILE_DIM + threadIdx.y;

    for (int j = 0; j < TILE_DIM; j += BLOCK_ROWS)
        AT[(y+j)*N + x] = tile[threadIdx.x][threadIdx.y + j];

}

void
parseArg (int argc, char** argv, int* N)
{
	if(argc == 2) {
		*N = atoi (argv[1]);
		assert (*N > 0);
	} else {
		fprintf (stderr, "usage: %s <N>\n", argv[0]);
		exit (EXIT_FAILURE);
	}
}

void
initArr (dtype* in, int N)
{
	int i;

	for(i = 0; i < N; i++) {
		in[i] = (dtype) rand () / RAND_MAX;
	}
}

void
cpuTranspose (dtype* A, dtype* AT, int N)
{
	int i, j;

	for(i = 0; i < N; i++) {
		for(j = 0; j < N; j++) {
			AT[j * N + i] = A[i * N + j];
		}
	}
}

int
cmpArr (dtype* a, dtype* b, int N)
{
	int cnt, i;

	cnt = 0;
	for(i = 0; i < N; i++) {
		if(abs(a[i] - b[i]) > 1e-6) cnt++;
	}

	return cnt;
}

void
gpuTranspose (dtype* A, dtype* AT, int N)
{
  struct stopwatch_t* timer = NULL;
  long double t_gpu;
  stopwatch_init ();
  timer = stopwatch_create ();
  
  stopwatch_start (timer);

	/* run your kernel here */
	dtype *d_A, *d_AT;
	CUDA_CHECK_ERROR(cudaMalloc((void**)&d_A, N * N * sizeof(dtype)));
	CUDA_CHECK_ERROR(cudaMalloc((void**)&d_AT, N * N * sizeof(dtype)));

	CUDA_CHECK_ERROR(cudaMemcpy(d_A, A, N * N * sizeof(dtype), cudaMemcpyHostToDevice));

	dim3 blockDims(TILE_DIM, BLOCK_ROWS, 1);
	dim3 gridDims((N + TILE_DIM - 1) / TILE_DIM, (N + TILE_DIM - 1) / TILE_DIM, 1);

	matTrans<<<gridDims, blockDims>>>(d_AT, d_A, N);

	CUDA_CHECK_ERROR(cudaMemcpy(AT, d_AT, N * N * sizeof(dtype), cudaMemcpyDeviceToHost));

	CUDA_CHECK_ERROR(cudaFree(d_A));
	CUDA_CHECK_ERROR(cudaFree(d_AT));

	/*end of my code*/

  cudaDeviceSynchronize ();
  t_gpu = stopwatch_stop (timer);
  fprintf (stdout, "GPU transpose: %Lg secs ==> %Lg billion elements/second\n",
           t_gpu, (N * N) / t_gpu * 1e-9 );

}

int 
main(int argc, char** argv)
{
	dtype *A, *ATgpu, *ATcpu;
  int err;

	int N;

  struct stopwatch_t* timer = NULL;
  long double t_cpu;

	N = -1;
	parseArg (argc, argv, &N);

  ATcpu = (dtype*) malloc (N * N * sizeof (dtype));
  ATgpu = (dtype*) malloc (N * N * sizeof (dtype));

  A = (dtype*) malloc (N * N * sizeof (dtype));

	initArr (A, N * N);

	gpuTranspose (A, ATgpu, N);

  stopwatch_init ();
  timer = stopwatch_create ();

	stopwatch_start (timer);
	cpuTranspose (A, ATcpu, N);
  t_cpu = stopwatch_stop (timer);
  fprintf (stdout, "Time to execute CPU transpose kernel: %Lg secs\n",
           t_cpu);

	err = cmpArr (ATgpu, ATcpu, N * N);
	if(err) {
		fprintf (stderr, "Transpose failed: %d\n", err);
	} else {
		fprintf (stdout, "Transpose successful\n");
	}

	free (A);
	free (ATgpu);
	free (ATcpu);

  return 0;
}
