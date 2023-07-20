#include "sort.hh"
#include <cstdlib>
#include <cstring>


static void merge(keytype* A, int nA, keytype* B, int nB, keytype* C) {
  int i = 0, j = 0, k = 0;

  while (i < nA && j < nB) {
    if (A[i] <= B[j]) {
      C[k++] = A[i++];
    } else {
      C[k++] = B[j++];
    }
  }

  while (i < nA) {
    C[k++] = A[i++];
  }

  while (j < nB) {
    C[k++] = B[j++];
  }
}

static void mergeSort(keytype* A, int n, keytype* B) {
  if (n == 1) {
    return;
  }

  int nA = n / 2;
  int nB = n - nA;

  mergeSort(A, nA, B);
  mergeSort(A + nA, nB, B);

  merge(A, nA, A + nA, nB, B);

  memcpy(A, B, n * sizeof(keytype));
}

void mySort(int n, keytype* A) {
  keytype* B = newKeys(n);

  mergeSort(A, n, B);

  free(B);
}
