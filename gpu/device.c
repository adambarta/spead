#include <stdio.h>

#include "shared.h"

#define KERNELS_FILE  "/kernels.cl"
#define KERNELDIR     "./"
#define LEN 1024

int main(int argc, char *argv[])
{
  struct ocl_ds     *o_ds;
  cl_int err;
  cl_event evt;
  cl_mem            o_in;
  cl_int4           *o_out;
  struct ocl_kernel *o_k;
  int len = LEN;
  
  size_t workGroupSize[1];
  
  workGroupSize[0] = len;

  o_ds = create_ocl_ds(KERNELDIR KERNELS_FILE);
  if (o_ds == NULL){
    return 1;
  }

  o_in = create_ocl_mem(o_ds, sizeof(cl_int4)*len);
  if (o_in == NULL){
    goto free_mem_in;
  }
  
  o_out = malloc(sizeof(cl_int4) * len);
  if (o_out == NULL){
    goto free_mem_out;
  }

  o_k = create_ocl_kernel(o_ds, "ocl_layout");
  if (o_k == NULL){
    goto free_kernel;
  }

#if 0
  err  = clSetKernelArg(o_k->k_kernel, 0, sizeof(cl_mem), (void *) &o_in);
  err |= clSetKernelArg(o_k->k_kernel, 1, sizeof(int), (void *) &len);
  if (err != CL_SUCCESS){
#ifdef DEBUG
    fprintf(stderr, "clSetKernelArg return %s\n", oclErrorString(err));
#endif
    goto clean_up;
  }

  err = clEnqueueNDRangeKernel(o_ds->d_cq, o_k->k_kernel, 1, NULL, workGroupSize, NULL, 0, NULL, &evt);
  if (err != CL_SUCCESS){
#ifdef DEBUG
    fprintf(stderr, "clEnqueueNDRangeKernel: %s\n", oclErrorString(err));
#endif
    goto clean_up;
  }

  clReleaseEvent(evt);
  clFinish(o_ds->d_cq);

  if (xfer_from_ocl_mem(o_ds, o_in, sizeof(cl_int4) * len, o_out) < 0){
#ifdef DEBUG
    fprintf(stderr, "%s: xfer from ocl error\n", __func__);
#endif
    goto clean_up;
  }

#ifdef DEBUG
  int i;
  for (i=0; i<len; i++){
    fprintf(stderr, "%d %d %d %d\n", o_out[i].w, o_out[i].x, o_out[i].y, o_out[i].z);
  }
#endif

#endif

clean_up:


  destroy_ocl_kernel(o_k);
free_kernel:
  if (o_out)
    free(o_out);
free_mem_out:
  destroy_ocl_mem(o_in);
free_mem_in:
  destroy_ocl_ds(o_ds);

  return 0;
}