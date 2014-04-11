/*********************
 *
 *    Yichao Xue
 *    yichaox
 *
 *********************/



/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{

    REQUIRES(M > 0);
    REQUIRES(N > 0);

if(M == 32){
    int n, m, i, j;
    //out loop: divide the matrix into 4X4 smaller ones
    for(n = 0; n < 4; n++){
        for(m = 0; m < 4; m++){
            //smaller matrix which is at diagonal
            if(m == n){
                for(i = 0; i < 8; i++){
                    for(j = 0; j <8; j++){
                        if(i != j)
                            B[m * 8 +j][n * 8 + i] = A[n * 8 + i][m * 8 +j];  
                        }
                        B[n * 8 + i][n * 8 + i] = A[n * 8 + i][n * 8 + i];
                    }
                }else{
                    // m != n
                    for(i = 0; i < 8; i++){
                        for(j = 0; j <8; j++){
                            B[m * 8 +j][n * 8 + i] = A[n * 8 + i][m * 8 +j];
                    }
                }
            }
        }
    }
   // ENSURES(is_transpose(M, N, A, B));
}
// M=64
/* divide the matrix into 64(8x8) small matrice */
if(M == 64){
    int x,y;  //index for 8x8 matrix
    int i,j;  //index for small matrix
    for(x = 0; x < 64; x+=8){
        for(y = 0; y < 64; y+=8){
            if(x < 56 && y < 56){

                for(i = 0; i < 4; i++){
                    for(j = 0; j < 4; j++){
                        B[y + j][x + i]= A[x + i][y +j];  //transpose 
                    }
                    for(j = 4; j < 8; j++){
                        B[60 + i][52 + j ] = A[x + i][y + j];//move 
                    }      
                }

                for(i = 4; i < 8; i++){
                    for(j = 0; j < 4; j++){
                        B[y + j][x + i]= A[x + i][y +j];  //transpose
                    }
                    for(j = 4; j < 8; j++){
                        B[56 + i][56 + j ] = A[x + i][y + j];//move
                    }
                }

                for(i = 0; i < 4; i++){
                    for(j = 4; j < 8; j++){
                        B[y +j][x+i]=B[60 + i][52 + j ];//copy
                    }
                }

                for(i = 4; i < 8; i++){
                    for(j = 4; j < 8; j++){
                        B[y + j ][x + i]= B[56 + i][56 + j ];//copy
                    }
                }
            }else{

                for(i = 0; i < 4; i++){
                    for(j = 0; j < 4; j++){
                        B[y + j][x + i] = A[x + i][y + j];
                    }
                }

                for(i = 4; i < 8; i++){
                    for(j = 0; j < 4; j++){
                        B[y + j ][x + i] = A[x + i][y + j];
                    }
                }

                 for(i = 4; i < 8; i++){
                    for(j = 4; j < 8; j++){
                        B[y + j ][x + i] = A[x + i][y + j];
                    }
                }

                 for(i = 0; i < 4; i++){
                    for(j = 4; j < 8; j++){
                        B[y + j ][x + i] = A[x + i][y + j];
                    }
                }
            }
        }
    }
}

//M=61
if(M == 61){
    int x,y;
    int i;
    //divide the matrix into 67x56 and 67x5
    for(y = 0; y < 56; y+=8){
        for(x = 0; x < 67; x++){
            for(i=0; i < 8; i++){
                B[y+i][x] = A[x][y+i];
            }
        }
    }
    for(x = 0; x < 67; x++){
        for(i=0; i<5; i++){
                B[56+i][x]=A[x][56+i];
        }
    }

}
ENSURES(is_transpose(M, N, A, B));
}


/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

