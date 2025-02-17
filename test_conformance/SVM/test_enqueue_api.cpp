//
// Copyright (c) 2017 The Khronos Group Inc.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "common.h"
#include "harness/mt19937.h"

#include <vector>
#include <atomic>

#if !defined(_WIN32)
#include <unistd.h>
#endif 

typedef struct
{
  std::atomic<cl_uint> status;
  cl_uint num_svm_pointers;
  std::vector<void *> svm_pointers;
} CallbackData;

void generate_data(std::vector<cl_uchar> &data, size_t size, MTdata seed)
{
  cl_uint randomData = genrand_int32(seed);
  cl_uint bitsLeft = 32;

  for( size_t i = 0; i < size; i++ )
  {
    if( 0 == bitsLeft)
    {
      randomData = genrand_int32(seed);
      bitsLeft = 32;
    }
    data[i] = (cl_uchar)( randomData & 255 );
    randomData >>= 8; randomData -= 8;
  }
}

//callback which will be passed to clEnqueueSVMFree command
void CL_CALLBACK callback_svm_free(cl_command_queue queue, cl_uint num_svm_pointers, void * svm_pointers[], void * user_data)
{
  CallbackData *data = (CallbackData *)user_data;
  data->num_svm_pointers = num_svm_pointers;
  data->svm_pointers.resize(num_svm_pointers, 0);

  cl_context context;
  if(clGetCommandQueueInfo(queue, CL_QUEUE_CONTEXT, sizeof(cl_context), &context, 0) != CL_SUCCESS)
  {
    log_error("clGetCommandQueueInfo failed in the callback\n");
    return;
  }

  for (size_t i = 0; i < num_svm_pointers; ++i)
  {
    data->svm_pointers[i] = svm_pointers[i];
    clSVMFree(context, svm_pointers[i]);
  }

  data->status.store(1, std::memory_order_release);
}

int test_svm_enqueue_api(cl_device_id deviceID, cl_context c, cl_command_queue queue, int num_elements)
{
  clContextWrapper context = NULL;
  clCommandQueueWrapper queues[MAXQ];
  cl_uint num_devices = 0;
  const size_t elementNum = 1024;
  const size_t numSVMBuffers = 32;
  cl_int error = CL_SUCCESS;
  RandomSeed seed(0);

  error = create_cl_objects(deviceID, NULL, &context, NULL, &queues[0], &num_devices, CL_DEVICE_SVM_COARSE_GRAIN_BUFFER);
  if(error) return -1;

  queue = queues[0];

  //all possible sizes of vectors and scalars
  size_t typeSizes[] = {
    sizeof(cl_uchar),
    sizeof(cl_uchar2),
    sizeof(cl_uchar3),
    sizeof(cl_uchar4),
    sizeof(cl_uchar8),
    sizeof(cl_uchar16),
    sizeof(cl_ushort),
    sizeof(cl_ushort2),
    sizeof(cl_ushort3),
    sizeof(cl_ushort4),
    sizeof(cl_ushort8),
    sizeof(cl_ushort16),
    sizeof(cl_uint),
    sizeof(cl_uint2),
    sizeof(cl_uint3),
    sizeof(cl_uint4),
    sizeof(cl_uint8),
    sizeof(cl_uint16),
    sizeof(cl_ulong),
    sizeof(cl_ulong2),
    sizeof(cl_ulong3),
    sizeof(cl_ulong4),
    sizeof(cl_ulong8),
    sizeof(cl_ulong16),
  };

  for (size_t i = 0; i < ( sizeof(typeSizes) / sizeof(typeSizes[0]) ); ++i)
  {
    //generate initial data
    std::vector<cl_uchar> fillData0(typeSizes[i]), fillData1(typeSizes[i], 0), fillData2(typeSizes[i]);
    generate_data(fillData0, typeSizes[i], seed);
    generate_data(fillData2, typeSizes[i], seed);

    cl_uchar *srcBuffer = (cl_uchar *)clSVMAlloc(context, CL_MEM_READ_WRITE, elementNum * typeSizes[i], 0);
    cl_uchar *dstBuffer = (cl_uchar *)clSVMAlloc(context, CL_MEM_READ_WRITE, elementNum * typeSizes[i], 0);

    clEventWrapper userEvent = clCreateUserEvent(context, &error);
    test_error(error, "clCreateUserEvent failed");

    clEventWrapper eventMemFill;
    error = clEnqueueSVMMemFill(queue, srcBuffer, &fillData0[0], typeSizes[i], elementNum * typeSizes[i], 1, &userEvent, &eventMemFill);
    test_error(error, "clEnqueueSVMMemFill failed");

    clEventWrapper eventMemcpy;
    error = clEnqueueSVMMemcpy(queue, CL_FALSE, dstBuffer, srcBuffer, elementNum * typeSizes[i], 1, &eventMemFill, &eventMemcpy);
    test_error(error, "clEnqueueSVMMemcpy failed");

    error = clSetUserEventStatus(userEvent, CL_COMPLETE);
    test_error(error, "clSetUserEventStatus failed");

    clEventWrapper eventMap;
    error = clEnqueueSVMMap(queue, CL_FALSE, CL_MAP_READ | CL_MAP_WRITE, dstBuffer, elementNum * typeSizes[i], 1, &eventMemcpy, &eventMap);
    test_error(error, "clEnqueueSVMMap failed");

    error = clWaitForEvents(1, &eventMap);
    test_error(error, "clWaitForEvents failed");

    //data verification
    for (size_t j = 0; j < elementNum * typeSizes[i]; ++j)
    {
      if (dstBuffer[j] != fillData0[j % typeSizes[i]])
      {
        log_error("Invalid data at index %ld, expected %d, got %d\n", j, fillData0[j % typeSizes[i]], dstBuffer[j]);
        return -1;
      }
    }

    clEventWrapper eventUnmap;
    error = clEnqueueSVMUnmap(queue, dstBuffer, 0, 0, &eventUnmap);
    test_error(error, "clEnqueueSVMUnmap failed");

    error = clEnqueueSVMMemFill(queue, srcBuffer, &fillData2[0], typeSizes[i], elementNum * typeSizes[i] / 2, 0, 0, 0);
    test_error(error, "clEnqueueSVMMemFill failed");

    error = clEnqueueSVMMemFill(queue, dstBuffer + elementNum * typeSizes[i] / 2, &fillData2[0], typeSizes[i], elementNum * typeSizes[i] / 2, 0, 0, 0);
    test_error(error, "clEnqueueSVMMemFill failed");

    error = clEnqueueSVMMemcpy(queue, CL_FALSE, dstBuffer, srcBuffer, elementNum * typeSizes[i] / 2, 0, 0, 0);
    test_error(error, "clEnqueueSVMMemcpy failed");

    error = clEnqueueSVMMemcpy(queue, CL_TRUE, dstBuffer + elementNum * typeSizes[i] / 2, srcBuffer + elementNum * typeSizes[i] / 2, elementNum * typeSizes[i] / 2, 0, 0, 0);
    test_error(error, "clEnqueueSVMMemcpy failed");

    void *ptrs[] = {(void *)srcBuffer, (void *)dstBuffer};

    clEventWrapper eventFree;
    error = clEnqueueSVMFree(queue, 2, ptrs, 0, 0, 0, 0, &eventFree);
    test_error(error, "clEnqueueSVMFree failed");

    error = clWaitForEvents(1, &eventFree);
    test_error(error, "clWaitForEvents failed");

    //event info verification for new SVM commands
    cl_command_type commandType;
    error = clGetEventInfo(eventMemFill, CL_EVENT_COMMAND_TYPE, sizeof(cl_command_type), &commandType, NULL);
    test_error(error, "clGetEventInfo failed");
    if (commandType != CL_COMMAND_SVM_MEMFILL)
    {
      log_error("Invalid command type returned for clEnqueueSVMMemFill\n");
      return -1;
    }

    error = clGetEventInfo(eventMemcpy, CL_EVENT_COMMAND_TYPE, sizeof(cl_command_type), &commandType, NULL);
    test_error(error, "clGetEventInfo failed");
    if (commandType != CL_COMMAND_SVM_MEMCPY)
    {
      log_error("Invalid command type returned for clEnqueueSVMMemcpy\n");
      return -1;
    }

    error = clGetEventInfo(eventMap, CL_EVENT_COMMAND_TYPE, sizeof(cl_command_type), &commandType, NULL);
    test_error(error, "clGetEventInfo failed");
    if (commandType != CL_COMMAND_SVM_MAP)
    {
      log_error("Invalid command type returned for clEnqueueSVMMap\n");
      return -1;
    }

    error = clGetEventInfo(eventUnmap, CL_EVENT_COMMAND_TYPE, sizeof(cl_command_type), &commandType, NULL);
    test_error(error, "clGetEventInfo failed");
    if (commandType != CL_COMMAND_SVM_UNMAP)
    {
      log_error("Invalid command type returned for clEnqueueSVMUnmap\n");
      return -1;
    }

    error = clGetEventInfo(eventFree, CL_EVENT_COMMAND_TYPE, sizeof(cl_command_type), &commandType, NULL);
    test_error(error, "clGetEventInfo failed");
    if (commandType != CL_COMMAND_SVM_FREE)
    {
      log_error("Invalid command type returned for clEnqueueSVMFree\n");
      return -1;
    }
  }

  std::vector<void *> buffers(numSVMBuffers, 0);
  for(size_t i = 0; i < numSVMBuffers; ++i) buffers[i] = clSVMAlloc(context, CL_MEM_READ_WRITE, elementNum, 0);

  //verify if callback is triggered correctly
  CallbackData data;
  data.status = 0;

  error = clEnqueueSVMFree(queue, buffers.size(), &buffers[0], callback_svm_free, &data, 0, 0, 0);
  test_error(error, "clEnqueueSVMFree failed");

  error = clFinish(queue);
  test_error(error, "clFinish failed");

  //wait for the callback
  while(data.status.load(std::memory_order_acquire) == 0) { 
    usleep(1);
  }

  //check if number of SVM pointers returned in the callback matches with expected
  if (data.num_svm_pointers != buffers.size())
  {
    log_error("Invalid number of SVM pointers returned in the callback, expected: %ld, got: %d\n", buffers.size(), data.num_svm_pointers);
    return -1;
  }

  //check if pointers returned in callback are correct
  for (size_t i = 0; i < buffers.size(); ++i)
  {
    if (data.svm_pointers[i] != buffers[i])
    {
      log_error("Invalid SVM pointer returned in the callback, idx: %ld\n", i);
      return -1;
    }
  }

  return 0;
}