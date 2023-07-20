#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <omp.h>
#include <math.h>

#include "sort.hh"

/* A helper function to merge two sorted sub-arrays into one sorted array */
void merge(keytype* A, keytype* B, int low, int mid, int high) {
    int i = low, j = mid + 1, k = low;

    while (i <= mid && j <= high) {
        if (A[i] <= A[j])
            B[k++] = A[i++];
        else
            B[k++] = A[j++];
    }

    while (i <= mid)
        B[k++] = A[i++];

    while (j <= high)
        B[k++] = A[j++];

    for (k = low; k <= high; ++k)
        A[k] = B[k];
}

/* The recursive function to split and sort the sub-arrays */
void parallel_mergesort(keytype* A, keytype* B, int low, int high, int max_depth) {
    if (low >= high)
        return;

    if (max_depth == 0) {
        std::sort(A + low, A + high + 1);
        return;
    }

    int mid = low + (high - low) / 2;

    #pragma omp taskgroup
    {
        #pragma omp task shared(A, B)
        parallel_mergesort(A, B, low, mid, max_depth - 1);

        #pragma omp task shared(A, B)
        parallel_mergesort(A, B, mid + 1, high, max_depth - 1);
    }

    merge(A, B, low, mid, high);
}

/* The main sorting routine, which calls the parallel_mergesort() function */
void mySort(int N, keytype* A) {
    int max_depth = static_cast<int>(log2(N)) - 2;
    if (max_depth < 0) max_depth = 0;

    keytype* B = newKeys(N);
    #pragma omp parallel
    #pragma omp single
    parallel_mergesort(A, B, 0, N - 1, max_depth);

    free(B);
}
