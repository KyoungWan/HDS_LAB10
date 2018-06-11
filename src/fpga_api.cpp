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
  int col_cnt=(int)ceil(float(M)/64);
  int row_cnt=(int)ceil(float(N)/64);
  int col = col_cnt *64;
  int row = row_cnt *64;

  float *new_mat= (float*)malloc(col*row*sizeof(float));
  float *new_vec= (float*)malloc(row*sizeof(float));
  //copy large_mat, input into matrix
  //make all 0 , redundant by malloc??????????
  memset(new_mat, 0, col*row*sizeof(float));	
  memset(new_vec, 0, row*sizeof(float));	
  for(int i=0; i<N; i++) {
    for(int j=0; j<M; j++){
      *(new_mat+row_cnt*SIZE*i+j) = *(large_mat+M*i +j );
    }
  }

  for(int i=0; i<M; i++) {
    *(new_vec+i) = *(input+i);
  }

  // write down your code here.

  float *temp = (float*)malloc(N*sizeof(float));
  memset(temp, 0, N*sizeof(float));

  const float* ptemp;
  float *tempMat = (float*)malloc(SIZE*SIZE*sizeof(float));

  for(int i=0; i<col; i=i+SIZE) {
    for(int j=0; j<row; j=j+SIZE){
      int m=i;
      int n=j;
      for(int t=0; t<SIZE*SIZE; t++) {
        tempMat[t] = *(new_mat+m*col+n);
        n++;
        if((t+1)%SIZE==0) { m++; n=j;}
      }
      memcpy(mat, tempMat, (SIZE*SIZE)*sizeof(float));
      memcpy(vec, new_vec+j, (SIZE)*sizeof(float));
      ptemp = this->run();
      int idx=0;
      for(int k=i; k<i+SIZE; k++){
        *(temp+k) = *(temp+k)+*(ptemp+idx++);
      }
    }
  }
  memcpy(output, temp, N*sizeof(float));
  output = temp;
}
