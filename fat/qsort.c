#include "qsort.h"

static u16 x, y, z, w;

void randInit(u16 a, u16 b, u16 c, u16 d) {
	x = a;
	y = b;
	z = c;
	w = d;
}

static inline u16 randGenerate(void) {
	u16 t;
	t = x ^ (x << 5);
	x = y; y = z; z = w;
	return w = w ^ (w >> 11) ^ t ^ (t >> 4);
}

static inline void swap(int *array, u16 x, u16 y) {
	const int temp = array[x];
	array[x] = array[y];
	array[y] = temp;
}

static inline u16 partition(int *array, u16 startIndex, u16 endIndex) {
	u16 i = startIndex, j = i + 1;
	const int pivot = array[startIndex];
	while ( j < endIndex ) {
		if ( array[j] < pivot ) {
			i++;
			swap(array, i, j);
		}
		j++;
	}
	array[startIndex] = array[i];
	array[i] = pivot;
	return i;
}

static inline void insertionSort(int *array, u16 startIndex, u16 endIndex) {
	u16 i, j;
	int key;
	for ( j = startIndex + 1; j < endIndex; j++ ) {
		key = array[j];
		i = j;
		if ( i > 0 && array[i-1] > key ) {
			do {
				array[i] = array[i-1];
				i--;
			} while ( i > 0 && array[i-1] > key );
			array[i] = key;
		}
	}
}

void quickSort(int *array, u16 startIndex, u16 endIndex) {
	const u16 len = endIndex - startIndex;
	if ( len > 2 ) {
		u16 pivot;
		const u32 pivotIndex = startIndex + ((randGenerate() * len) >> 16);
		swap(array, startIndex, pivotIndex);
		pivot = partition(array, startIndex, endIndex);
		quickSort(array, startIndex, pivot);
		quickSort(array, pivot+1, endIndex);
	} else {
		insertionSort(array, startIndex, endIndex);
	}
}
