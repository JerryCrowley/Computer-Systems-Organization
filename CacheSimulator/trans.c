/*
* Jeremiah Crowley
* jcc608
*
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

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
* transpose_submit - This is the solution transpose function that you
* will be graded on for Part B of the assignment. Do not change
* the description string "Transpose submission", as the driver
* searches for that string to identify the transpose function to
* be graded.
*/
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{

	int cacheBlockSize;

	if( (M==32 && N==32) || (M==64 && N==64))
        {		

		if(M==32 && N ==32)
			cacheBlockSize = 8;
		else
			cacheBlockSize = 4;
 
                for (int column = 0; column < N; column += cacheBlockSize) //Matrix is going to be broken up into either 8x8 or 4x4 squares with the diagonal as the 0,0
		{       
                        for (int row = 0; row < M; row += cacheBlockSize) //In order to make 8x8 square, need 8 columns and 8 rows or 4x4 for 4. 
			{
                                if(column==row) //If the column and the row are the same, then we are at the 0,0 coordinate of the blocked matirx
                                {
                                        for (int i = column; i < column+cacheBlockSize; i++) 
					{
                                                for (int j = row; j < row+cacheBlockSize ;j++)
						{
                                                        B[j][i] = A[i][j]; //Flip the coordinates of the blocked matrix
						}
				
						continue;
					}
                                }

                                else //If we are not at a diagonal, just flip coordinates 
                                {
                                        for (int i = column; i < column+cacheBlockSize; i++)
					{
                                                for (int j = row; j < row+cacheBlockSize ;j++)
						{
                                                        B[j][i] = A[i][j];
						}
					}
                                }
			}
		}
        }
    

        if(M==61 && N==67)
    	{
		cacheBlockSize = 18; 
                for (int column = 0; column < N; column += cacheBlockSize) //Similar to the other two matrices
		{
                        for (int row = 0; row < M; row += cacheBlockSize)
			{
                                for (int i = column; i < column+cacheBlockSize; i++) 
				{
                                        for (int j = row; j < row+cacheBlockSize ;j++)
					{
                                                if(i<67 && j<61) 
						{
                                                        B[j][i] = A[i][j];
						}
					}
				}
			}
		}
        }

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

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

}

/*
* registerFunctions - This function registers your transpose
* functions with the driver. At runtime, the driver will
* evaluate each of the registered functions and summarize their
* performance. This is a handy way to experiment with different
* transpose strategies.
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
* A. You can check the correctness of your transpose by calling
* it before returning from the transpose function.
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
