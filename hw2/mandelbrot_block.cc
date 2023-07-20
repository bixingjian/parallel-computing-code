/**
 *  \file mandelbrot_block.cc
 *
 *  \brief Implement your parallel mandelbrot set in this file.
 */

#include <iostream>
#include <cstdlib>
#include <mpi.h>
#include "render.hh"


int mandelbrot(double x, double y) {
  int maxit = 511;
  double cx = x;
  double cy = y;
  double newx, newy;

  int it = 0;
  for (it = 0; it < maxit && (x*x + y*y) < 4; ++it) { // | | <  2
    newx = x*x - y*y + cx;
    newy = 2*x*y + cy;
    x = newx;
    y = newy;
  }
  return it;
}

int main(int argc, char* argv[]) {
  double minX = -2.1;
  double maxX = 0.7;
  double minY = -1.25;
  double maxY = 1.25;

  int rank_block, world_size;
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank_block);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  

  int height, width;
  if (argc == 3) {
    height = atoi (argv[1]);
    width = atoi (argv[2]);
    assert (height > 0 && width > 0);
  } else {
    fprintf (stderr, "usage: %s <height> <width>\n", argv[0]);
    fprintf (stderr, "where <height> and <width> are the dimensions of the image.\n");
    return -1;
  }

  double start, end, timer;
  start = MPI_Wtime();

  double it = (maxX - minX)/width;
  double jt = (maxY - minY)/height;
  double x, y;

  // get the param for MPI_Gatherv
  int start_row = height * rank_block / world_size;
  int end_row = height * (rank_block + 1) / world_size;
  if (rank_block == world_size - 1) { // check for the edge case
    end_row = height;
  }

  int sendcount = (end_row - start_row) * width; // may not equal to world_size. for the last one.
  double* sendbuf = (double*) malloc(sizeof(double) * sendcount);
  y = minY + jt * start_row;
  for (int j = 0; j < end_row - start_row; ++j) { 
    for (int i = 0; i < width; ++i) {
      sendbuf[j *  width+i] = mandelbrot(x, y) / 512.0;
      x += it;
    }
    y += jt;
  }

  double* recv_buf = (double*) malloc(sizeof(double) * height * width);
  int* recv_counts = (int*) malloc(sizeof(int) * world_size);
  for (int i = 0; i < world_size; i++) {
    int start_index = height * i / world_size
    int end_index = height * (i +1) / world_size;
    recv_counts[i] = width * (end_index - start_index);
  } 

  int* displs = (int*) malloc(sizeof(int) * world_size);
  displs[0] = 0;
  for (int i = 0; i < world_size; i++) {
    displs[i] = displs[i-1] + recv_counts[i-1];
  }

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Gatherv(sendbuf, sendcount, MPI_DOUBLE, recv_buf, recv_counts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD); //sendbuf sendcount sendtype recvcounts displs recvtype root comm


  if (rank_block == 0) {
    gil::rgb8_image_t img(height, width);
    auto img_view = gil::view(img);
    y = minY;
    for (int j = 0; j < height; ++j) {
      x = minX;
      for (int i = 0; i < width; i++) {
        img_view(i, j) = render(recv_buf[j * width + i]);
        x += it;
      }
      y += jt;
    }
    gil::png_write_view("mandelbrot_block.png", const_view(img));
  }

  if (rank_block == 0) {
    delete(sendbuf);
    delete(recv_buf);
    delete(recv_counts);
    delete(displs);
  }

  end = MPI_Wtime();
  timer = end - start;
  if (rank_block == 0) {
    printf("\nMPI_Wtime measured a time to be: %1.3f\n", end-start);fflush(stdout);
  }

  MPI_Finalize();
  return 0;
}
