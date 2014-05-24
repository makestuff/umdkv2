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

static inline void swap(CVPtr *array, u16 x, u16 y) {
	const CVPtr temp = array[x];
	array[x] = array[y];
	array[y] = temp;
}

static inline u16 partition(CVPtr *array, u16 startIndex, u16 endIndex, CompareFunc compFunc) {
	const CVPtr pivot = array[startIndex];
	u16 i = startIndex, j = i + 1;
	while ( j < endIndex ) {
		if ( compFunc(array[j], pivot) < 0 ) {
			i++;
			swap(array, i, j);
		}
		j++;
	}
	array[startIndex] = array[i];
	array[i] = pivot;
	return i;
}

/*static inline void insertionSort(const void **array, u16 startIndex, u16 endIndex, CompareFunc compFunc) {
	u16 i, j;
	CVPtr key;
	for ( j = startIndex + 1; j < endIndex; j++ ) {
		key = array[j];
		i = j;
		if ( i > 0 && compFunc(array[i-1], key) > 0 ) {
			do {
				array[i] = array[i-1];
				i--;
			} while ( i > 0 && compFunc(array[i-1], key) > 0 );
			array[i] = key;
		}
	}
}*/

void quickSort(CVPtr *array, u16 startIndex, u16 endIndex, CompareFunc compFunc) {
	const u16 len = endIndex - startIndex;
	if ( len > 1 ) {
		u16 pivot;
		const u16 pivotIndex = (u16)(startIndex + ((randGenerate() * len) >> 16));
		swap(array, startIndex, pivotIndex);
		pivot = partition(array, startIndex, endIndex, compFunc);
		quickSort(array, startIndex, pivot, compFunc);
		quickSort(array, pivot+1, endIndex, compFunc);
	}
	//else {
	//	insertionSort(array, startIndex, endIndex, compFunc);
	//}
}
