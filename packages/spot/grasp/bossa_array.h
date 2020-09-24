// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#ifndef bossa_array_h
#define bossa_array_h

#include <stdio.h>
#include <stdlib.h>

template <class T> class BossaArrayUnit {
 public:
  T value;
  int mark;
  BossaArrayUnit () {mark = 0;}
};

template <class T> class BossaArray {
 private:
  int globalmark;
  BossaArrayUnit <T> *array;
  T defvalue; //default value
  int size;

 public:
  T getDefault () {return defvalue;}
  inline int getSize() {return size;}

  /* careful: default value will be undefined here */
  BossaArray (int maxindex) {
    array = new BossaArrayUnit <T> [maxindex+1];
    size = maxindex;
    globalmark = 1;
  }


  BossaArray (int maxindex, T value) {
    array = new BossaArrayUnit <T> [maxindex+1];
    size = maxindex;
    globalmark = 1;
    defvalue = value;
  }

  inline T get (int x) {return getValue(x);}

  inline T getValue (int x) {
#ifdef BOSSA_DEBUG
    if (x > size) {
      fprintf (stderr, "BossaArray::getValue: index out of bounds (%d > %d).\n", x, size);
      exit (-1);
    }
#endif
    if (globalmark == array[x].mark) return array[x].value;
    else return defvalue;
  }

  inline void setValue (int x, T v) {
#ifdef BOSSA_DEBUG
    if (x > size) {
      fprintf (stderr, "Array index out of bounds (%d > %d).\n", x, size);
      exit (-1);
    }
#endif
    array[x].value = v;
    array[x].mark = globalmark;		
  }

  void reset() {globalmark++;}

  void reset (int maxindex, T value) {
    defvalue = value;
    if (maxindex != size) {
      delete [] array;
      size = maxindex;
      array = new BossaArrayUnit <T> [size + 1];
      globalmark = 1;
      defvalue = value;
    } else reset();
  }

  void reset (T value) {
    defvalue = value;
    reset();
  }

  void copyFrom (BossaArray <T> *other) {
    reset (other->getSize(), other->getDefault());
    for (int i = 1; i <= getSize(); i++) {
      this->setValue (i, other->getValue(i));
    }
  }

  ~BossaArray () {delete [] array;}

};

#endif

