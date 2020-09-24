// This is third-party software that is distributed with the TEVA-SPOT Toolkit.
// Usage of this code is covered by an ATT Commercial License.

#ifndef bossa_stack_h
#define bossa_stack_h

#include <stdio.h>
#include <stdlib.h>

template <class T> class BossaStack {
 private:
  T *stack;
  int top, size;

  void fatal (const char *func, const char *msg) {
    fprintf (stderr, "BossaStack::%s: %s\n", func, msg);
    exit (-1);
  }

 public:
  bool isFull() {return (top==size);}

  void push (T i) {
    top++; 
    if (top>size) fatal ("push", "stack full");
    stack[top] = i;
  }

  T pop() {
    if (isEmpty()) fatal ("pop", "stack empty"); 
    return (stack[top--]);
  };

  bool isEmpty() {return (top==0);};

  int getNElements() {return top;}

  T peek (int p) {
    if ((p<1)||(p>top)) fatal ("peek", "index out of bounds");
    return (stack[p]);
  }

  BossaStack (int s) {
    if (s<1) fatal ("BossaStack", "invalid size");
    top = 0;
    size = s;
    stack = new T [size+1];
  };

  ~BossaStack () {delete [] stack;};
};

#endif








