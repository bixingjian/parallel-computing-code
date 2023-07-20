/**
 *  \file mandelbrot_dynamic.cc
 *
 *  \brief Implement your parallel mandelbrot set in this file.
 */

#include <iostream>
#include <cstdlib>
#include <mpi.h>
#include <chrono>
#include "render.hh"

#define WORK_TAG 1
#define RESULT_TAG 2
#define TERMINATE_TAG 3

using namespace std;
using namespace chrono;

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

void coordinator(int rank, int size, int height, int width) {
  MPI_Status status;
  double* result = new double[height * width];
  int rows_sent = 0;
  int rows_received = 0;

  while (rows_received < height) {
    // Send work to idle workers
    while (rows_sent < height && rows_sent < size - 1) {
      MPI_Send(&rows_sent, 1, MPI_INT, rows_sent + 1, WORK_TAG, MPI_COMM_WORLD);
      rows_sent++;
    }

    // Receive results from workers
    double* recv_buffer = new double[width + 1];
    MPI_Recv(recv_buffer, width + 1, MPI_DOUBLE, MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &status);

    int received_row = recv_buffer[0];
    memcpy(result + received_row * width, recv_buffer + 1, sizeof(double) * width);
    rows_received++;

    delete[] recv_buffer;
  }

  gil::rgb8_image_t img(height, width);
  auto img_view = gil::view(img);

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      img_view(i, j) = render(result[j * width + i]);
    }
  }

  gil::png_write_view("mandelbrot_dynamic.png", const_view(img));

  delete[] result;
}

void worker(int rank, int height, int width) {
  MPI_Status status;
  double* send_buffer = new double[width + 1];
  int row;

  while (true) {
    MPI_Recv(&row, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    if (status.MPI_TAG == TERMINATE_TAG) {
      break;
    }

    send_buffer[0] = row;
    double y = -1.25 + (2.5 / height) * row;

    for (int i = 0; i < width; i++) {
      double x = -2.1 + (2.8 / width) * i;
      send_buffer[i + 1] = mandelbrot(x, y) / 512.0;
    }

    MPI_Send(send_buffer, width + 1, MPI_DOUBLE, 0, RESULT_TAG, MPI_COMM_WORLD);
  }

  delete[] send_buffer;
}

int main(int argc, char* argv[]) {
  int rank, size;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int height, width;
  if (argc == 3) {
    height = atoi(argv[1]);
    width = atoi(argv[2]);
    assert(height > 0 && width > 0);
  } else {
    fprintf(stderr, "usage: %s <height> <width>\n", argv[0]);
    fprintf(stderr, "where <height> and <width> are the dimensions of the image.\n");
    return -1;
  }

  if (rank == 0) {
    auto start = MPI_Wtime();
    coordinator(rank, size, height, width);
    auto end = MPI_Wtime();
    printf("\nMPI_Wtime measured a time to be: %1.3f\n", end-start);fflush(stdout);
  } else {
    worker(rank, height, width);
  }

  // Send termination signal to workers
  if (rank == 0) {
    for (int i = 1; i < size; i++) {
      MPI_Send(nullptr, 0, MPI_INT, i, TERMINATE_TAG, MPI_COMM_WORLD);
    }
  }

  MPI_Finalize();
  printf("done");
}
