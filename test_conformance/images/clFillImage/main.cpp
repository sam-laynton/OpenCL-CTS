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
#include "../harness/compat.h"

#include <stdio.h>
#include <string.h>

#if !defined(_WIN32)
#include <unistd.h>
#include <sys/time.h>
#endif

#include "../testBase.h"
#include "../harness/testHarness.h"

bool gDebugTrace;
bool gTestSmallImages;
bool gTestMaxImages;
bool gTestRounding;
bool gEnablePitch;
int  gTypesToTest;
cl_channel_type  gChannelTypeToUse = (cl_channel_type)-1;
cl_channel_order gChannelOrderToUse = (cl_channel_order)-1;
cl_device_type   gDeviceType = CL_DEVICE_TYPE_DEFAULT;

extern int test_image_set( cl_device_id device, cl_context context, cl_command_queue queue, MethodsToTest testMethod );
static void printUsage( const char *execName );

#define MAX_ALLOWED_STD_DEVIATION_IN_MB        8.0

int test_1D(cl_device_id device, cl_context context, cl_command_queue queue, int num_elements)
{
    return test_image_set(device, context, queue, k1D);
}
int test_2D(cl_device_id device, cl_context context, cl_command_queue queue, int num_elements)
{
    return test_image_set(device, context, queue, k2D);
}
int test_3D(cl_device_id device, cl_context context, cl_command_queue queue, int num_elements)
{
    return test_image_set(device, context, queue, k3D);
}
int test_1Darray(cl_device_id device, cl_context context, cl_command_queue queue, int num_elements)
{
    return test_image_set(device, context, queue, k1DArray);
}
int test_2Darray(cl_device_id device, cl_context context, cl_command_queue queue, int num_elements)
{
    return test_image_set(device, context, queue, k2DArray);
}

test_definition test_list[] = {
    ADD_TEST( 1D ),
    ADD_TEST( 2D ),
    ADD_TEST( 3D ),
    ADD_TEST( 1Darray ),
    ADD_TEST( 2Darray ),
};

const int test_num = ARRAY_SIZE( test_list );

int main(int argc, const char *argv[])
{
    cl_channel_type  chanType;
    cl_channel_order chanOrder;

    checkDeviceTypeOverride( &gDeviceType );

    const char ** argList = (const char **)calloc( argc, sizeof( char*) );

    if( NULL == argList )
    {
        log_error( "Failed to allocate memory for argList array.\n" );
        return 1;
    }

    argList[0] = argv[0];
    size_t argCount = 1;

    // Parse arguments
    for ( int i = 1; i < argc; i++ )
    {
        if ( strcmp( argv[i], "cpu" ) == 0 || strcmp( argv[i], "CL_DEVICE_TYPE_CPU" ) == 0 )
            gDeviceType = CL_DEVICE_TYPE_CPU;
        else if ( strcmp( argv[i], "gpu" ) == 0 || strcmp( argv[i], "CL_DEVICE_TYPE_GPU" ) == 0 )
            gDeviceType = CL_DEVICE_TYPE_GPU;
        else if ( strcmp( argv[i], "accelerator" ) == 0 || strcmp( argv[i], "CL_DEVICE_TYPE_ACCELERATOR" ) == 0 )
            gDeviceType = CL_DEVICE_TYPE_ACCELERATOR;
        else if ( strcmp( argv[i], "CL_DEVICE_TYPE_DEFAULT" ) == 0 )
            gDeviceType = CL_DEVICE_TYPE_DEFAULT;

        else if ( strcmp( argv[i], "debug_trace" ) == 0 )
            gDebugTrace = true;

        else if ( strcmp( argv[i], "small_images" ) == 0 )
            gTestSmallImages = true;
        else if ( strcmp( argv[i], "max_images" ) == 0 )
            gTestMaxImages = true;

        else if ( strcmp( argv[i], "use_pitches" ) == 0 )
            gEnablePitch = true;

        else if( strcmp( argv[i], "int" ) == 0 )
            gTypesToTest |= kTestInt;
        else if( strcmp( argv[i], "uint" ) == 0 )
            gTypesToTest |= kTestUInt;
        else if( strcmp( argv[i], "float" ) == 0 )
            gTypesToTest |= kTestFloat;

        else if ( strcmp( argv[i], "--help" ) == 0 || strcmp( argv[i], "-h" ) == 0 )
        {
            printUsage( argv[ 0 ] );
            return -1;
        }

        else if ( ( chanType = get_channel_type_from_name( argv[i] ) ) != (cl_channel_type)-1 )
            gChannelTypeToUse = chanType;

        else if ( ( chanOrder = get_channel_order_from_name( argv[i] ) ) != (cl_channel_order)-1 )
            gChannelOrderToUse = chanOrder;
        else
        {
            argList[argCount] = argv[i];
            argCount++;
        }
    }

    if ( gTypesToTest == 0 )
        gTypesToTest = kTestAllTypes;

    if ( gTestSmallImages )
        log_info( "Note: Using small test images\n" );

    int ret = runTestHarness( argCount, argList, test_num, test_list, true, false, 0 );

    if (gTestFailure == 0) {
        if (gTestCount > 1)
            log_info("PASSED %d of %d sub-tests.\n", gTestCount, gTestCount);
        else
            log_info("PASSED sub-test.\n");
    }
    else if (gTestFailure > 0) {
        if (gTestCount > 1)
            log_error("FAILED %d of %d sub-tests.\n", gTestFailure, gTestCount);
        else
            log_error("FAILED sub-test.\n");
    }

    free(argList);
    return ret;
}

static void printUsage( const char *execName )
{
    const char *p = strrchr( execName, '/' );
    if ( p != NULL )
        execName = p + 1;

    log_info( "Usage: %s [options] [test_names]\n", execName );
    log_info( "Options:\n" );
    log_info( "\tThe following flags specify the types to test. They can be combined; if none are specified, all are tested:\n" );
    log_info( "\t\tint - Test integer fill\n" );
    log_info( "\t\tuint - Test unsigned integer fill\n" );
    log_info( "\t\tfloat - Test float fill\n" );
    log_info( "\n" );
    log_info( "\trandomize - Uses random seed\n" );
    log_info( "\tdebug_trace - Enables additional debug info logging\n" );
    log_info( "\tsmall_images - Runs every format through a loop of widths 1-13 and heights 1-9, instead of random sizes\n" );
    log_info( "\tmax_images - Runs every format through a set of size combinations with the max values, max values - 1, and max values / 128\n" );
    log_info( "\tuse_pitches - Enables row and slice pitches\n" );
    log_info( "\n" );
    log_info( "Test names:\n" );
    for( int i = 0; i < test_num; i++ )
    {
        log_info( "\t%s\n", test_list[i].name );
    }
    log_info( "\n" );
    log_info( "You may also use appropriate CL_ channel type and ordering constants.\n" );
}
