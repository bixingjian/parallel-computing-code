/**
 *  \file mandelbrot_cyclic.cc
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
  MPI_Init(&argc, &argv);
  int rank_cyc, world_size; // set the rank and size
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank_cyc);

  double minX = -2.1;
  double maxX = 0.7;
  double minY = -1.25;
  double maxY = 1.25;

  // printf("rank: %d, world %d", rank_cyc, world_size);

  
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

  double it = (maxX - minX)/width; // changing for the normal complex plane.
  double jt = (maxY - minY)/height;
  double x, y;

  int P_interval = world_size;
  int p = rank_cyc;

  int row_times = ((height - p  - 1) / world_size) +1;
  int sendcount = row_times * width; // compute the size of the distributed strips.

  double* sendbuf = (double*) malloc(sizeof(double) * sendcount);
  y = minY + jt * p;
  for (int j = p; j < height; j+= P_interval) { 
    x = minX;
    for (int i = 0; i < width; ++i) {
      sendbuf[j / P_interval * width + i] =  mandelbrot(x, y) / 512.0;
      x += it;
    }
    y += jt * P_interval;
  }

  double* recvbuf = (double*) malloc(sizeof(double) * height * width);
  int* recv_counts = (int*) malloc(sizeof(int) * world_size);
    for (int i = 0; i < world_size; i++) {
    recv_counts[i] = ((height - i -1) / world_size + 1)* width;
  }

  int* displs = (int*) malloc(sizeof(int) * world_size);
  displs[0] = 0;
  for (int i = 0; i < world_size; i++) {
    displs[i] = displs[i-1] + recv_counts[i-1];
  }


  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Gatherv(sendbuf, sendcount, MPI_DOUBLE, recvbuf, recv_counts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);//sendbuf sendcount sendtype recvcounts displs recvtype root comm


  if (rank_cyc == 0) { // get the image after gathering
    gil::rgb8_image_t img(height, width);
    auto img_view = gil::view(img);
    y = minY;
    for (int j = 0; j < height; ++j) {
      x = minX;
      int x_index = j / world_size * width + displs[j % world_size];
      for (int i = 0; i < width; i++) {
        img_view(i, j) = render(recvbuf[x_index + i]);
        x += it;
      }
      y += jt;
    }
    gil::png_write_view("mandelbrot_cyclic.png", const_view(img));
  }

  if (rank_cyc == 0) {
    delete(sendbuf);
    delete(recvbuf);
    delete(recv_counts);
    delete(displs);
  }

  end = MPI_Wtime();
  timer = end - start;
  if (rank_cyc == 0) {
    printf("MPI_Wtime measured a time to be: %1.3f\n", end-start);fflush(stdout);
  }

  MPI_Finalize();
  return 0;
}

