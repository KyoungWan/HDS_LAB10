#include "fpga_api.h"
#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <math.h>
#include <cstdio>
#include <cstdlib>

#define DATA_SIZE SIZE*(SIZE+1)*sizeof(float) // fpga bram data size

#define min(x,y) (((x)<(y))?(x):(y))

FPGA::FPGA(off_t data_addr, off_t api_addr)
{
    fd_ = open("/dev/mem", O_RDWR);
    data_ = static_cast<float*>(mmap(NULL, DATA_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd_, data_addr));
    api_ = static_cast<unsigned int*>(mmap(NULL, sizeof(unsigned int), PROT_READ|PROT_WRITE, MAP_SHARED,fd_, api_addr));
}

FPGA::~FPGA()
{
    munmap(data_, DATA_SIZE );
    munmap(api_, sizeof(unsigned int));
    close(fd_);
}

float* FPGA::matrix(void)
{
	return data_ + SIZE;
}

float* FPGA::vector(void)
{
	return data_;
}

const float* __attribute__((optimize("O0"))) FPGA::run()
{
    *api_ = 0x5555;
    while(*api_ == 0x5555);

    return data_;
}

void FPGA::largeMV(const float* large_mat, const float* input,
		float* output, int M, int N)
{
    // write down your code here.

    float* vec = this->vector(); // 64
    float* mat = this->matrix(); // 64 * 64

    //making multiple of 64
    int col_cnt=(int)ceil(M/64);
    int row_cnt=(int)ceil(N/64);
    int col = col_cnt *64;
    int row = row_cnt *64;

    float *new_mat= (float*)malloc(col*row*sizeof(float));
    float *new_vec= (float*)malloc(row*sizeof(float));
    //copy large_mat, input into matrix
    //make all 0 , redundant by malloc??????????
    for(int i=0; i<row_cnt*SIZE; i++) {
      for(int j=0; j<col_cnt*SIZE; j++){
        *(new_mat+row_cnt*SIZE*i+j) = 0;
      }
    }
    for(int i=0; i<M; i++) {
      for(int j=0; j<N; j++){
        *(new_mat+row_cnt*SIZE*i+j) = *(large_mat+M*i +j );
      }
    }
    //printf
    printf("print Matrix start \n");
    for(int i=0; i<col_cnt*SIZE; i++) {
      for(int j=0; j<row_cnt*SIZE; j++){
        //printf("[%d][%d]=%.f ", i, j, *(new_mat+row_cnt*i +j));
        printf("%.1f ", *(new_mat+row_cnt*i +j));
      }
      printf("\n");
    }
    printf("print Matrix end \n");
    // write down your code here.
    float *temp = (float*)malloc(M*sizeof(float));
    memset(temp, 0, M*sizeof(float));

    const float* ptemp;
    for(int i=0; i<col; i=i+SIZE) {
      for(int j=0; j<row; j=j+SIZE){
        memcpy(mat, new_mat+i*col+j, (SIZE*SIZE)*sizeof(float));
        memcpy(vec, new_vec+i, (SIZE)*sizeof(float));
        ptemp = this->run();

        for(int i=0+SIZE*j; i<SIZE+SIZE*j; i++){
          *(temp+i) = *(temp+i)+*(ptemp+i);
        }
      }
    }
    output = temp;
}
