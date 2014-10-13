/**
* Implementation of the Lyra2 Password Hashing Scheme (PHS). SSE-oriented implementation.
*
* Author: The Lyra PHC team (http://www.lyra2.net/) -- 2014.
*
* This software is hereby placed in the public domain.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
#include "Lyra2.h"
#include "Sponge.h"

#if (nPARALLEL > 1)
#include <omp.h>
#endif

/**
 * Executes Lyra2 based on the G function from Blake2b or BlaMka. The number of columns of the memory matrix is set to nCols = 256.
 * This version supports salts and passwords whose combined length is smaller than the size of the memory matrix,
 * (i.e., (nRows x nCols x b) bits, where "b" is the underlying sponge's bitrate). In this implementation, the "params" 
 * is composed by all integer parameters (treated as type "unsigned int") in the order they are provided, plus the value 
 * of nCols, (i.e., params = kLen || pwdlen || saltlen || timeCost || nRows || nCols).
 *
 * @param out The derived key to be output by the algorithm
 * @param outlen Desired key length
 * @param in User password
 * @param inlen Password length
 * @param salt Salt
 * @param saltlen Salt length
 * @param t_cost Parameter to determine the processing time (T)
 * @param m_cost Memory cost parameter (defines the number of rows of the memory matrix, R)
 *
 * @return 0 if the key is generated correctly; -1 if there is an error (usually due to lack of memory for allocation)
 */
int PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt, size_t saltlen, unsigned int t_cost, unsigned int m_cost){
    return LYRA2(out, outlen, in, inlen, salt, saltlen, t_cost, m_cost, N_COLS);
}

void print128(__m128i *v){
    uint64_t *v1 = malloc(16 * sizeof (uint64_t));
    int i;

    _mm_store_si128( (__m128i *)&v1[0], (__m128i)v[0]);
    _mm_store_si128( (__m128i *)&v1[2], (__m128i)v[1]);
    _mm_store_si128( (__m128i *)&v1[4], (__m128i)v[2]);
    _mm_store_si128( (__m128i *)&v1[6], (__m128i)v[3]);
    _mm_store_si128( (__m128i *)&v1[8], (__m128i)v[4]);
    _mm_store_si128( (__m128i *)&v1[10], (__m128i)v[5]);
    _mm_store_si128( (__m128i *)&v1[12], (__m128i)v[6]);
    _mm_store_si128( (__m128i *)&v1[14], (__m128i)v[7]);

    for (i = 0; i < 16; i++) {
        printf("%ld|",v1[i]);
    }
    printf("\n");
}

#if (nPARALLEL == 1)
/**
 * Executes Lyra2 based on the G function from Blake2b or BlaMka. This version supports salts and passwords
 * whose combined length is smaller than the size of the memory matrix, (i.e., (nRows x nCols x b) bits,
 * where "b" is the underlying sponge's bitrate). In this implementation, the "params" is composed by all 
 * integer parameters (treated as type "unsigned int") in the order they are provided, plus the value 
 * of nCols, (i.e., params = kLen || pwdlen || saltlen || timeCost || nRows || nCols).
 *
 * @param K The derived key to be output by the algorithm
 * @param kLen Desired key length
 * @param pwd User password
 * @param pwdlen Password length
 * @param salt Salt
 * @param saltlen Salt length
 * @param timeCost Parameter to determine the processing time (T)
 * @param nRows Number or rows of the memory matrix (R)
 * @param nCols Number of columns of the memory matrix (C)
 *
 * @return 0 if the key is generated correctly; -1 if there is an error (usually due to lack of memory for allocation)
 */
int LYRA2(void *K, unsigned int kLen, const void *pwd, unsigned int pwdlen, const void *salt, unsigned int saltlen, unsigned int timeCost, unsigned int nRows, unsigned int nCols){

    //============================= Basic variables ============================//
    uint64_t row0 = 3;          //row0: sequentially written during Setup; randomly picked during Wandering
    uint64_t prev0 = 2;         //prev0: stores the previous value of row0
    uint64_t row1 = 1;          //row1: revisited during Setup, and then read [and written]; randomly picked during Wandering
    uint64_t prev1 = 0;         //prev1: stores the previous value of row1
    
    uint64_t tau;               //Time Loop iterator

    int64_t gap = 1;            //Modifier to the step, assuming the values 1 or -1
    uint64_t step = 1;          //Visitation step (used during Setup to dictate the sequence in which rows are read)
    uint64_t window = 2;        //Visitation window (used to define which rows can be revisited during Setup)
    
    uint64_t i;                 //auxiliary iteration counter
    //==========================================================================/

    //========== Initializing the Memory Matrix and pointers to it =============//
    //Tries to allocate enough space for the whole memory matrix
    i = (uint64_t) ((uint64_t)nRows * (uint64_t)ROW_LEN_BYTES);
    __m128i *wholeMatrix = malloc(i);
    if (wholeMatrix == NULL) {
        return -1;
    }
    //Allocates pointers to each row of the matrix
    __m128i **memMatrix = malloc(nRows * sizeof (uint64_t*));
    if (memMatrix == NULL) {
        return -1;
    }
    //Places the pointers in the correct positions
    __m128i *ptrWord = wholeMatrix;
    for (i = 0; i < nRows; i++) {
        memMatrix[i] = ptrWord;
        ptrWord += ROW_LEN_INT128;
    }
    //==========================================================================/

    //============= Padding (password + salt + params) with 10*1 ===============//

    //OBS.:The memory matrix will temporarily hold the password: not for saving memory,
    //but this ensures that the password copied locally will be overwritten as soon as possible

    //First, we clean enough blocks for the password, salt, params and padding
    uint64_t nBlocksInput = ((saltlen + pwdlen + 6 * sizeof (int)) / BLOCK_LEN_BLAKE2_SAFE_BYTES) + 1;
    byte *ptrByte = (byte*) wholeMatrix;
    memset(ptrByte, 0, nBlocksInput * BLOCK_LEN_BLAKE2_SAFE_BYTES);

    //Prepends the password
    memcpy(ptrByte, pwd, pwdlen);
    ptrByte += pwdlen;

    //Concatenates the salt
    memcpy(ptrByte, salt, saltlen);
    ptrByte += saltlen;

    //Concatenates the params: every integer passed as parameter, in the order they are provided by the interface
    memcpy(ptrByte, &kLen, sizeof (int));
    ptrByte += sizeof (int);
    memcpy(ptrByte, &pwdlen, sizeof (int));
    ptrByte += sizeof (int);
    memcpy(ptrByte, &saltlen, sizeof (int));
    ptrByte += sizeof (int);
    memcpy(ptrByte, &timeCost, sizeof (int));
    ptrByte += sizeof (int);
    memcpy(ptrByte, &nRows, sizeof (int));
    ptrByte += sizeof (int);
    memcpy(ptrByte, &nCols, sizeof (int));
    ptrByte += sizeof (int);

    //Now comes the padding
    *ptrByte = 0x80; //first byte of padding: right after the password
    ptrByte = (byte*) wholeMatrix; //resets the pointer to the start of the memory matrix
    ptrByte += nBlocksInput * BLOCK_LEN_BLAKE2_SAFE_BYTES - 1; //sets the pointer to the correct position: end of incomplete block
    *ptrByte ^= 0x01; //last byte of padding: at the end of the last incomplete block
    
    //==========================================================================/

    //============== Initializing the Sponge State =============/
    //Sponge state: 8 __m128i, BLOCK_LEN_INT128 words of them for the bitrate (b) and the remainder for the capacity (c)
    __m128i *state = malloc(8 * sizeof (__m128i));
    if (state == NULL) {
	return -1;
    }
    initState(state);
    
    //========================================================//
    //============= Absorbing the input data with the sponge ===============//
    
    //Absorbing salt, password and params: this is the only place in which the block length is hard-coded to 512 bits, for compatibility with Blake2b and BlaMka
    ptrWord = wholeMatrix;
    for (i = 0; i < nBlocksInput; i++) {
	absorbBlockBlake2Safe(state, ptrWord);          //absorbs each block of pad(pwd || salt || params)
	ptrWord += BLOCK_LEN_BLAKE2_SAFE_BYTES;         //goes to next block of pad(pwd || salt || params)
    }
    
    //================================ Setup Phase =============================//

    //Initializes M[0]
    reducedSqueezeRow0(state, memMatrix[0]); //The locally copied password is most likely overwritten here
    //Initializes M[1]
    reducedDuplexRow1(state, memMatrix[0], memMatrix[1]);
    //Initializes M[2]
    reducedDuplexRow2(state, memMatrix[0], memMatrix[1], memMatrix[2]);
    
    for(row0 = 3 ; row0 < nRows; row0++){
	//M[row0][N_COLS-1-col] = M[prev0][col] XOR rand; //M[row1] = M[row1] XOR rotRt(rand)
	reducedDuplexRowFilling(state, memMatrix[row1], memMatrix[prev0], memMatrix[prev1], memMatrix[row0]);

        //Updates the "prev" indices: the rows more recently updated
        prev0 = row0;
        prev1 = row1;
        
        //updates the value of row1: deterministically picked, with a variable step
        row1 = (row1 + step) & (window - 1);
	
	//Checks if all rows in the window where visited.
	if (row1 == 0) {
	    step = window + gap;        //changes the step: approximately doubles its value
	    window *= 2;                //doubles the size of the re-visitation window
	    gap = -gap;                 //inverts the modifier to the step 
	}
    }
    
    //============================ Wandering Phase =============================//
    unsigned int randomColumn = 0;
    for (tau = 1; tau <= timeCost; tau++) {
	for (i = 0 ; i < nRows ; i++) {            
            //Selects a pseudorandom indices row0 and row1
	    //------------------------------------------------------------------------------------------
	    /*(USE THIS IF nRows IS A POWER OF 2)*/
            //row0 = ((uint64_t)(((__uint128_t *)state)[0])) & (nRows-1);	
            //row1 = ((uint64_t)(((__uint128_t *)state)[1])) & (nRows-1);
            /*(USE THIS FOR THE "GENERIC" CASE)*/
            row0 = ((uint64_t)(((__uint128_t *)state)[0])) % nRows;	
            row1 = ((uint64_t)(((__uint128_t *)state)[1])) % nRows;                        
            
	    //Performs a reduced-round duplexing operation over M[row0] [+] M[row1] [+] M[prev0] [+] M[prev1], updating both M[row0] and M[row1]
            //M[row0][col] = M[row0][col] XOR rand; M[row1][col] = M[row1][col] XOR rotRt(rand)
	    randomColumn = reducedDuplexRowWandering(state, memMatrix[row0], memMatrix[row1], memMatrix[prev0], memMatrix[prev1]);

	    //update prev's: they now point to the last rows ever updated
	    prev0 = row0;
            prev1 = row1;   
	}
    }
    //==========================================================================/

    //============================ Wrap-up Phase ===============================//
    //Absorbs one last block of the memory matrix with the full-round sponge
    absorbRandomColumn(state, memMatrix[row0], randomColumn);
    
    //Squeezes the key with the full-round sponge
    squeeze(state, K, kLen);
    //==========================================================================/

    //========================= Freeing the memory =============================//
    free(memMatrix);
    free(wholeMatrix);

    //Wiping out the sponge's internal state before freeing it
    memset(state, 0, 8 * sizeof (__m128i));
    free(state);
    //==========================================================================/


    return 0;
}
#endif


#if (nPARALLEL > 1)
/**
 * Executes Lyra2 based on the G function from Blake2b or BlaMka. This version supports salts and passwords
 * whose combined length is smaller than the size of the memory matrix, (i.e., (nRows x nCols x b) bits,
 * where "b" is the underlying sponge's bitrate). In this implementation, the "params" is composed by all 
 * integer parameters (treated as type "unsigned int") in the order they are provided, plus the value 
 * of nCols, (i.e., params = kLen || pwdlen || saltlen || timeCost || nRows || nCols).
 *
 * @param K The derived key to be output by the algorithm
 * @param kLen Desired key length
 * @param pwd User password
 * @param pwdlen Password length
 * @param salt Salt
 * @param saltlen Salt length
 * @param timeCost Parameter to determine the processing time (T)
 * @param nRows Number or rows of the memory matrix (R)
 * @param nCols Number of columns of the memory matrix (C)
 *
 * @return 0 if the key is generated correctly; -1 if there is an error (usually due to lack of memory for allocation)
 */
int LYRA2(void *K, unsigned int kLen, const void *pwd, unsigned int pwdlen, const void *salt, unsigned int saltlen, unsigned int timeCost, unsigned int nRows, unsigned int nCols){

    //============================= Basic variables ============================//
    uint64_t i,j;        //auxiliary iteration counter
    
    //==========================================================================/

    //========== Initializing the Memory Matrix and pointers to it =============//
    //Allocates pointers to each row of the matrix
    __m128i **memMatrix = malloc(nRows * sizeof (uint64_t*));
    if (memMatrix == NULL) {
        return -1;
    }
    //Allocates pointers to each key
    unsigned char **pKeys = malloc(nPARALLEL * sizeof (unsigned char*));
    if (pKeys == NULL) {
        return -1;
    }
    
#if _OPENMP == 201107  //OpenMP 3.1
    #pragma omp parallel num_threads(nPARALLEL) default(none) /*private(pwd)*/ shared(memMatrix,  pKeys, pwd, pwdlen, salt, saltlen, nRows, nCols, kLen, timeCost)
#endif // _OPENMP

#if _OPENMP == 201307  //OpenMP 4.0
    #pragma omp parallel proc_bind(spread) num_threads(nPARALLEL) default(none) /*private(pwd)*/ shared(memMatrix,  pKeys, pwd, pwdlen, salt, saltlen, nRows, nCols, kLen, timeCost)
#endif // _OPENMP
    {
        //============================= Basic threads variables ============================//
        uint64_t tau;                    //Time Loop iterator
        uint64_t step = 1;               //Visitation step (used during Setup and Wandering phases)
        uint64_t window = 2;             //Visitation window (used to define which rows can be revisited during Setup)
        int64_t gap = 1;                //Modifier to the step, assuming the values 1 or -1

        uint64_t row0 = 3;          //row0: sequentially written during Setup; randomly picked during Wandering
        uint64_t prev0 = 2;         //prev0: stores the previous value of row0
        uint64_t row1 = 1;          //row1: revisited during Setup, and then read [and written]; randomly picked during Wandering
        uint64_t prev1 = 0;         //prev1: stores the previous value of row1
        uint64_t row0P;
        uint64_t j0;

        uint64_t threadNumber = 0;
        uint64_t iP;
        uint64_t jP;                     //Starts with threadNumber.
        uint64_t kP;
        
        uint64_t sizeSlicedRows;
        uint64_t sync = 1;
        int sideA, sideB;
        //==========================================================================/

        //========================== BootStrapping Phase ==========================//
        // Size of each chunk that each thread will work with
        sizeSlicedRows = nRows/nPARALLEL;
        // Thread index:
        threadNumber = omp_get_thread_num();
        
        uint64_t sliceStart = threadNumber*sizeSlicedRows;
        uint64_t halfSlice = sizeSlicedRows/2;

        iP = (uint64_t) ((uint64_t) sizeSlicedRows * (uint64_t) ROW_LEN_BYTES);
        __m128i *threadSliceMatrix = malloc(iP);
        if (threadSliceMatrix == NULL) {
            printf("Error: unable to allocate memory (nRows too large?)\n");
            exit(EXIT_FAILURE);
        }
        //Places the pointers in the correct positions
        __m128i *ptrWord = threadSliceMatrix;
        for (kP = 0; kP < sizeSlicedRows; kP++) {
            memMatrix[threadNumber*sizeSlicedRows + kP] = ptrWord;
            ptrWord += ROW_LEN_INT128;
        }

        unsigned char *threadKey =  malloc(kLen);
        if (threadKey == NULL) {
            exit(EXIT_FAILURE);
        }

        //Places the pointers in the correct positions
        pKeys[threadNumber] = threadKey;


        //============= Padding (password + salt + params) with 10*1 ===============//

        //OBS.:The memory matrix will temporarily hold the password: not for saving memory,
        //but this ensures that the password copied locally will be overwritten as soon as possible

        //First, we clean enough blocks for the password, salt, params and padding
        uint64_t nBlocksInput = ((saltlen + pwdlen + 6 * sizeof (int)) / BLOCK_LEN_BLAKE2_SAFE_BYTES) + 1;
        byte *ptrByte = (byte*) threadSliceMatrix;
        memset(ptrByte, 0, nBlocksInput * BLOCK_LEN_BLAKE2_SAFE_BYTES);

        //Prepends the password
        memcpy(ptrByte, pwd, pwdlen);
        ptrByte += pwdlen;

        //Concatenates the salt
        memcpy(ptrByte, salt, saltlen);
        ptrByte += saltlen;

        //Concatenates the basil: every integer passed as parameter, in the order they are provided by the interface
        memcpy(ptrByte, &kLen, sizeof (int));
        ptrByte += sizeof (int);
        memcpy(ptrByte, &pwdlen, sizeof (int));
        ptrByte += sizeof (int);
        memcpy(ptrByte, &saltlen, sizeof (int));
        ptrByte += sizeof (int);
        memcpy(ptrByte, &timeCost, sizeof (int));
        ptrByte += sizeof (int);
        memcpy(ptrByte, &nRows, sizeof (int));
        ptrByte += sizeof (int);
        memcpy(ptrByte, &nCols, sizeof (int));
        ptrByte += sizeof (int);

        //Now comes the padding
        *ptrByte = 0x80; //first byte of padding: right after the password
        ptrByte = (byte*) threadSliceMatrix; //resets the pointer to the start of the memory matrix
        ptrByte += nBlocksInput * BLOCK_LEN_BLAKE2_SAFE_BYTES - 1; //sets the pointer to the correct position: end of incomplete block
        *ptrByte ^= 0x01; //last byte of padding: at the end of the last incomplete block

        //==========================================================================/

        //============== Initializing the Sponge State =============/
        //Sponge state: 8 __m128i, BLOCK_LEN_INT128 words of them for the bitrate (b) and the remainder for the capacity (c)
        //Thread State
        __m128i *threadState = malloc(8 * sizeof (__m128i));
        if (threadState == NULL) {
            exit(EXIT_FAILURE);
        }
        initState(threadState);
        
        //============= Absorbing the input data with the sponge ===============//

        //Absorbing salt, password and params: this is the only place in which the block length is hard-coded to 512 bits, for compatibility with Blake2b and BlaMka
        ptrWord = threadSliceMatrix;
        for (kP = 0; kP < nBlocksInput; kP++) {
            absorbBlockBlake2Safe(threadState, ptrWord);        //absorbs each block of pad(pwd || salt || params)
            ptrWord += BLOCK_LEN_BLAKE2_SAFE_BYTES;             //goes to next block of pad(pwd || salt || params)
        }

        //Allocates the State Index to be absorbed later
        __m128i *stateIDX = malloc(BLOCK_LEN_BLAKE2_SAFE_BYTES);
        if (stateIDX == NULL) {
                exit(EXIT_FAILURE);
        }
        // Prepares the State Index to be absorbed
        //Now comes the padding
        //*stateIDX = 0;
        ptrByte = (byte*) stateIDX;
        memset(ptrByte, 0, BLOCK_LEN_BLAKE2_SAFE_BYTES);

        // Total of parallelism
        ptrByte = (byte*) stateIDX;

        ptrByte +=  BLOCK_LEN_BLAKE2_SAFE_BYTES/2 - 1;          //sets the pointer to the last position of half vector.
        *ptrByte = (byte) nPARALLEL;

        ptrByte = (byte*) stateIDX;                             //resets the pointer to the start of the memory matrix
        ptrByte += BLOCK_LEN_BLAKE2_SAFE_BYTES - 1;             //sets the pointer to the last position.

        //Different for each thread
        *ptrByte = (byte) threadNumber;
        
        // For each absorb the stateIDX is different
        absorbBlockBlake2Safe(threadState, stateIDX);

        //================================ Setup Phase =============================//
        //Initializes M[0]
        reducedSqueezeRow0(threadState, memMatrix[sliceStart]);               //The locally copied password is most likely overwritten here
        //Initializes M[1]
        reducedDuplexRow1(threadState, memMatrix[sliceStart], memMatrix[sliceStart+1]);
        //Initializes M[2]
        reducedDuplexRow2(threadState, memMatrix[sliceStart + 0], memMatrix[sliceStart + 1], memMatrix[jP*sizeSlicedRows + 2]);
        
        jP = threadNumber;
        
        uint64_t syncronize = sync*(sizeSlicedRows/SIGMA)-1;

        for (row0 = 3; row0 < sizeSlicedRows; row0++) {
            //M[row0][N_COLS-1-col] = M[prev0][col] XOR rand; //Mj[row1] = Mj[row1] XOR rotRt(rand)
            reducedDuplexRowFilling(threadState, memMatrix[jP*sizeSlicedRows + row1], memMatrix[sliceStart + prev0], memMatrix[jP*sizeSlicedRows + prev1], memMatrix[sliceStart + row0]);

            //Updates the "prev" indices: the rows more recently updated
            prev0 = row0;
            prev1 = row1;

            //updates the value of row1: deterministically picked, with a variable step
            row1 = (row1 + step) & (window - 1);

            //Checks if all rows in the window where visited.
            if (row1 == 0) {
                step = window + gap;    //changes the step: approximately doubles its value
                window *= 2;            //doubles the size of the re-visitation window
                gap = -gap;             //inverts the modifier to the step
                #pragma omp barrier
            }
            if (row0 >= syncronize) {
                sync++;
                syncronize = sync*(sizeSlicedRows/SIGMA)-1;
                jP = (jP + 1) % nPARALLEL;
                #pragma omp barrier
            }
        }
        
        
        // Needs all matrix done before starting Wandering Phase.
        #pragma omp barrier

        //================================ Wandering Phase =============================//
        sync = 1;
        window = halfSlice;
        syncronize = sync*(sizeSlicedRows/SIGMA)-1;
        prev0 = window-1;
        
        sideA = sync % 2;
        sideB = (sync + 1) % 2;
        for (tau = 1; tau <= timeCost; tau++) {
            for (iP = 0; iP < sizeSlicedRows; iP++){
                
                
                //Selects a pseudorandom indices row0 and row0P (row0 = LSW(rand) mod wnd and row0p = LSW(rotRt(rand)) mod wnd)
                //------------------------------------------------------------------------------------------
                /*(USE THIS IF window IS A POWER OF 2)*/
                //row0 = ((uint64_t)(((__uint128_t *)threadState)[0])) & (window-1);
                //row0P = ((uint64_t)(((__uint128_t *)threadState)[1])) & (window-1);
                /*(USE THIS FOR THE "GENERIC" CASE)*/
                row0 = ((uint64_t)(((__uint128_t *)threadState)[0])) % window;
                row0P = ((uint64_t)(((__uint128_t *)threadState)[1])) % window;
                
                //Selects a pseudorandom indices j0 (LSW(rotRt^2 (rand)) mod p)
                j0 = ((uint64_t)(((__uint128_t *)threadState)[2])) % nPARALLEL;
                
                //Performs a reduced-round duplexing operation over M[row0] [+] Mj[row0P] [+] M[prev0], updating both M[row0]
                //M[row0 + window*sideA][col] = M[row0 + window*sideA][col] XOR rand
                reducedDuplexRowWanderingParallel(threadState, memMatrix[sliceStart + row0 + (uint64_t)window*sideA], 
                                                        memMatrix[j0*sizeSlicedRows + row0P + (uint64_t)window*sideB], memMatrix[sliceStart + prev0 + (uint64_t)window*sideA]);

                if (iP >= syncronize) {
                    sync++;
                    syncronize = sync*(sizeSlicedRows/SIGMA)-1;
                    sideA = sync % 2;
                    sideB = (sync + 1) % 2;
                    #pragma omp barrier
                }
                //update prev: they now point to the last rows ever updated
                prev0 = row0;
            }
            #pragma omp barrier
        }
        //========================================================//

        //============================ Wrap-up Phase ===============================//
        //Absorbs one last block of the memory matrix with the full-round sponge
        absorbRandomColumn(threadState,  memMatrix[row0]);

        //Squeezes the key
        squeeze(threadState, threadKey, kLen);

        //========================= Freeing the thread memory =============================//
        free(threadSliceMatrix);
        free(stateIDX);

        //Wiping out the sponge's internal state before freeing it
        memset(threadState, 0, 8 * sizeof (__m128i));
        free(threadState);
    } // Parallelism End
    
    // XORs all Keys
    for (i = 1; i < nPARALLEL; i++) {
        for (j = 0; j < kLen; j++) {
            pKeys[0][j] ^= pKeys[i][j];
        }
    }

    // Returns in the correct variable
    memcpy(K, pKeys[0], kLen);


    //========================= Freeing the memory =============================//
    free(memMatrix);

    //Free each thread Key
    for (i = 0; i < nPARALLEL; i++) {
        free(pKeys[i]);
    }
    //Free the pointes to allKeys
    free(pKeys);
    
    //==========================================================================/

    return 0;
}
#endif