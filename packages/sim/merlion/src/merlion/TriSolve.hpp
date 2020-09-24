#ifndef MERLION_TRISOLVE_HPP__
#define MERLION_TRISOLVE_HPP__


/*///////////////////////////////////////////*/
/*// Solve U with multiple rhs */
/*///////////////////////////////////////////*/
/* solve UX=B where B is a sparse coord rhs matrix (**NxNRHS) and X is dense (**NxNRHS) solution matrix, U is sparse CSC format */
void usolvem(int N, int n_start, int n_stop, int nrhs, const float *Ux, const int *Ui, const int *Up, const int *Bi, const int *Bj, const float *Bx, int Bnnz, float *X);
void usolvem(int N, int n_start, int n_stop, int nrhs, const double *Ux, const int *Ui, const int *Up, const int *Bi, const int *Bj, const double *Bx, int Bnnz, double *X);

/* solve UX=B where X and B are dense (**NxNRHS) solution matrices, U is sparse CSC format */
void usolvem(int N, int n_start, int n_stop, int nrhs, const float *Ux, const int *Ui, const int *Up, const float *B, float *X);
void usolvem(int N, int n_start, int n_stop, int nrhs, const double *Ux, const int *Ui, const int *Up, const double *B, double *X);

/* solve UX=B where X is dense (**NxNRHS) matrix which is converted from rhs to solution, U is sparse CSC format */
void usolvem(int N, int n_start, int n_stop, int nrhs, const float *Ux, const int *Ui, const int *Up, float *X);
void usolvem(int N, int n_start, int n_stop, int nrhs, const double *Ux, const int *Ui, const int *Up, double *X);
/*///////////////////////////////////////////*/

/*///////////////////////////////////////////*/
/*// Solve U with single rhs */
/*///////////////////////////////////////////*//* solve Ux=b where b is a sparse rhs and x is dense, U is sparse CSC format */ 
void usolve(int N, int n_start, int n_stop, const float *Ux, const int *Ui, const int *Up, const int *bi, const float *bx, int bnnz, float *x);
void usolve(int N, int n_start, int n_stop, const double *Ux, const int *Ui, const int *Up, const int *bi, const double *bx, int bnnz, double *x);

/* solve Ux=b where x and b are dense, U is sparse CSC format */
void usolve(int N, int n_start, int n_stop, const float *Ux, const int *Ui, const int *Up, const float *b, float *x);
void usolve(int N, int n_start, int n_stop, const double *Ux, const int *Ui, const int *Up, const double *b, double *x);

/* solve Ux=b where x is dense and is converted from a rhs to a solution, U is sparse CSC format */
void usolve(int N, int n_start, int n_stop, const float *Ux, const int *Ui, const int *Up, float *x);
void usolve(int N, int n_start, int n_stop, const double *Ux, const int *Ui, const int *Up, double *x);
/*///////////////////////////////////////////*/

/*///////////////////////////////////////////*/
/*// Solve U_transpose with multiple rhs */
/*///////////////////////////////////////////*/
/* solve U'X=B where B is a sparse coord rhs matrix (**NxNRHS) and X is dense (**NxNRHS) solution matrix, U is sparse CSC format */
void utsolvem(int N, int n_start, int n_stop, int nrhs, const float *Ux, const int *Ui, const int *Up, const int *Bi, const int *Bj, const float *Bx, int Bnnz, float *X);
void utsolvem(int N, int n_start, int n_stop, int nrhs, const double *Ux, const int *Ui, const int *Up, const int *Bi, const int *Bj, const double *Bx, int Bnnz, double *X);

/* solve U'X=B X and B are dense (**NxNRHS) solution matrices, U is sparse CSC format*/
void utsolvem(int N, int n_start, int n_stop, int nrhs, const float *Ux, const int *Ui, const int *Up, const float *B, float *X);
void utsolvem(int N, int n_start, int n_stop, int nrhs, const double *Ux, const int *Ui, const int *Up, const double *B, double *X);

/* solve U'X=B where X is dense (**NxNRHS) matrix and is converted from rhs to solution, U is sparse CSC format */
void utsolvem(int N, int n_start, int n_stop, int nrhs, const float *Ux, const int *Ui, const int *Up, float *X);
void utsolvem(int N, int n_start, int n_stop, int nrhs, const double *Ux, const int *Ui, const int *Up, double *X);
/*///////////////////////////////////////////*/

/*///////////////////////////////////////////*/
/*// Solve U_tranpose with single rhs */
/*///////////////////////////////////////////*//* solve U'x=b where b is a sparse rhs and x is dense, U is sparse CSC format */ 
void utsolve(int N, int n_start, int n_stop, const float *Ux, const int *Ui, const int *Up, const int *bi, const float *bx, int bnnz, float *x);
void utsolve(int N, int n_start, int n_stop, const double *Ux, const int *Ui, const int *Up, const int *bi, const double *bx, int bnnz, double *x);

/* solve U'x=b where x and b are dense, U is sparse CSC format */
void utsolve(int N, int n_start, int n_stop, const float *Ux, const int *Ui, const int *Up, const float *b, float *x);
void utsolve(int N, int n_start, int n_stop, const double *Ux, const int *Ui, const int *Up, const double *b, double *x);

/* solve U'x=b where x is dense and is converted from a rhs to a solution, U is sparse CSC format */
void utsolve(int N, int n_start, int n_stop, const float *Ux, const int *Ui, const int *Up, float *x);
void utsolve(int N, int n_start, int n_stop, const double *Ux, const int *Ui, const int *Up, double *x);
/*///////////////////////////////////////////*/




/*///////////////////////////////////////////*/
/*// Solve L with multiple rhs */
/*///////////////////////////////////////////*/
/* solve LX=B where B is a sparse coord rhs matrix (**NxNRHS) and X is dense (**NxNRHS) solution matrix, L is sparse CSC format */
void lsolvem(int N, int n_start, int n_stop, int nrhs, const float *Lx, const int *Li, const int *Lp, const int *Bi, const int *Bj, const float *Bx, int Bnnz, float *X);
void lsolvem(int N, int n_start, int n_stop, int nrhs, const double *Lx, const int *Li, const int *Lp, const int *Bi, const int *Bj, const double *Bx, int Bnnz, double *X);

/* solve LX=B where X and B are dense (**NxNRHS) solution matrices, L is sparse CSC format */
void lsolvem(int N, int n_start, int n_stop, int nrhs, const float *Lx, const int *Li, const int *Lp, const float *B, float *X);
void lsolvem(int N, int n_start, int n_stop, int nrhs, const double *Lx, const int *Li, const int *Lp, const double *B, double *X);

/* solve LX=B where X is dense (**NxNRHS) matrix which is converted from rhs to solution, L is sparse CSC format */
void lsolvem(int N, int n_start, int n_stop, int nrhs, const float *Lx, const int *Li, const int *Lp, float *X);
void lsolvem(int N, int n_start, int n_stop, int nrhs, const double *Lx, const int *Li, const int *Lp, double *X);
/*///////////////////////////////////////////*/

/*///////////////////////////////////////////*/
/*// Solve L with single rhs */
/*///////////////////////////////////////////*//* solve Lx=b where b is a sparse rhs and x is dense, L is sparse CSC format */ 
void lsolve(int N, int n_start, int n_stop, const float *Lx, const int *Li, const int *Lp, const int *bi, const float *bx, int bnnz, float *x);
void lsolve(int N, int n_start, int n_stop, const double *Lx, const int *Li, const int *Lp, const int *bi, const double *bx, int bnnz, double *x);

/* solve Lx=b where x and b are dense, L is sparse CSC format */
void lsolve(int N, int n_start, int n_stop, const float *Lx, const int *Li, const int *Lp, const float *b, float *x);
void lsolve(int N, int n_start, int n_stop, const double *Lx, const int *Li, const int *Lp, const double *b, double *x);

/* solve Lx=b where x is dense and is converted from a rhs to a solution, L is sparse CSC format */
void lsolve(int N, int n_start, int n_stop, const float *Lx, const int *Li, const int *Lp, float *x);
void lsolve(int N, int n_start, int n_stop, const double *Lx, const int *Li, const int *Lp, double *x);
/*///////////////////////////////////////////*/

/*///////////////////////////////////////////*/
/*// Solve L_transpose with multiple rhs */
/*///////////////////////////////////////////*/
/* solve L'X=B where B is a sparse coord rhs matrix (**NxNRHS) and X is dense (**NxNRHS) solution matrix, L is sparse CSC format */
void ltsolvem(int N, int n_start, int n_stop, int nrhs, const float *Lx, const int *Li, const int *Lp, const int *Bi, const int *Bj, const float *Bx, int Bnnz, float *X);
void ltsolvem(int N, int n_start, int n_stop, int nrhs, const double *Lx, const int *Li, const int *Lp, const int *Bi, const int *Bj, const double *Bx, int Bnnz, double *X);

/* solve L'X=B X and B are dense (**NxNRHS) solution matrices, L is sparse CSC format*/
void ltsolvem(int N, int n_start, int n_stop, int nrhs, const float *Lx, const int *Li, const int *Lp, const float *B, float *X);
void ltsolvem(int N, int n_start, int n_stop, int nrhs, const double *Lx, const int *Li, const int *Lp, const double *B, double *X);

/* solve L'X=B where X is dense (**NxNRHS) matrix and is converted from rhs to solution, L is sparse CSC format */
void ltsolvem(int N, int n_start, int n_stop, int nrhs, const float *Lx, const int *Li, const int *Lp, float *X);
void ltsolvem(int N, int n_start, int n_stop, int nrhs, const double *Lx, const int *Li, const int *Lp, double *X);
/*///////////////////////////////////////////*/

/*///////////////////////////////////////////*/
/*// Solve L_tranpose with single rhs */
/*///////////////////////////////////////////*//* solve L'x=b where b is a sparse rhs and x is dense, L is sparse CSC format */ 
void ltsolve(int N, int n_start, int n_stop, const float *Lx, const int *Li, const int *Lp, const int *bi, const float *bx, int bnnz, float *x);
void ltsolve(int N, int n_start, int n_stop, const double *Lx, const int *Li, const int *Lp, const int *bi, const double *bx, int bnnz, double *x);

/* solve L'x=b where x and b are dense, L is sparse CSC format */
void ltsolve(int N, int n_start, int n_stop, const float *Lx, const int *Li, const int *Lp, const float *b, float *x);
void ltsolve(int N, int n_start, int n_stop, const double *Lx, const int *Li, const int *Lp, const double *b, double *x);

/* solve L'x=b where x is dense and is converted from a rhs to a solution, L is sparse CSC format */
void ltsolve(int N, int n_start, int n_stop, const float *Lx, const int *Li, const int *Lp, float *x);
void ltsolve(int N, int n_start, int n_stop, const double *Lx, const int *Li, const int *Lp, double *x);
/*///////////////////////////////////////////*/

#endif
