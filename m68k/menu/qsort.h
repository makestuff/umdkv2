#ifndef QSORT_H
#define QSORT_H

#include "types.h"

typedef const void *CVPtr;
typedef short (*CompareFunc)(CVPtr x, CVPtr y);
void randInit(u16 a, u16 b, u16 c, u16 d);
void quickSort(CVPtr *array, u16 startIndex, u16 endIndex, CompareFunc compFunc);

#endif
