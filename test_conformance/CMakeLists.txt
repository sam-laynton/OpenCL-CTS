# Remember current source directory (`test_conformance').
set( CLConf_Install_Base_Dir "${CMAKE_CURRENT_SOURCE_DIR}" )

add_subdirectory( allocations )
add_subdirectory( api )
add_subdirectory( atomics )
add_subdirectory( basic )
add_subdirectory( buffers )
add_subdirectory( commonfns )
add_subdirectory( compatibility )
add_subdirectory( compiler )
add_subdirectory( computeinfo )
add_subdirectory( contractions )
add_subdirectory( conversions )
if(D3D10_IS_SUPPORTED)
   add_subdirectory( d3d10 )
endif(D3D10_IS_SUPPORTED)
if(D3D11_IS_SUPPORTED)
   add_subdirectory( d3d11 )
endif(D3D11_IS_SUPPORTED)
add_subdirectory( device_partition )
add_subdirectory( events )
add_subdirectory( geometrics )
if(GL_IS_SUPPORTED)
   add_subdirectory( gl )
endif(GL_IS_SUPPORTED)
if(GLES_IS_SUPPORTED)
   add_subdirectory(gles)
endif(GLES_IS_SUPPORTED)
add_subdirectory( half )
add_subdirectory( headers )
add_subdirectory( images )
add_subdirectory( integer_ops )
add_subdirectory( math_brute_force )
add_subdirectory( mem_host_flags )
add_subdirectory( multiple_device_context )
add_subdirectory( printf )
add_subdirectory( profiling )
add_subdirectory( relationals )
add_subdirectory( select )
add_subdirectory( thread_dimensions )
add_subdirectory( vec_align )
add_subdirectory( vec_step )
add_subdirectory( c11_atomics )
add_subdirectory( device_execution )
add_subdirectory( non_uniform_work_group )
add_subdirectory( SVM )
add_subdirectory( generic_address_space )
add_subdirectory( subgroups )
add_subdirectory( workgroups )
add_subdirectory( pipes )
add_subdirectory( device_timer )
add_subdirectory( clcpp )
add_subdirectory( spirv_new )
add_subdirectory( spir )

set(CSV_FILES
    opencl_conformance_tests_21_full_spirv.csv
    opencl_conformance_tests_21_legacy_wimpy.csv
    opencl_conformance_tests_22.csv
    opencl_conformance_tests_generate_spirv.csv
    opencl_conformance_tests_conversions.csv
    opencl_conformance_tests_d3d.csv
    opencl_conformance_tests_full.csv
    opencl_conformance_tests_full_no_math_or_conversions.csv
    opencl_conformance_tests_math.csv
    opencl_conformance_tests_quick.csv
    opencl_conformance_tests_20_full.csv
    opencl_conformance_tests_20_full_no_math_or_conversions.csv
    opencl_conformance_tests_20_quick.csv
)
set(PY_FILES
    run_conformance.py
)

# Copy .csv files
foreach(FILE ${CSV_FILES})
    configure_file(${FILE} ${FILE} COPYONLY)
endforeach()

# Copy test run script
foreach(FILE ${PY_FILES})
    if(WIN32)
        configure_file(${FILE} ${FILE} COPYONLY)
    else(WIN32)
        # Copy to CMakeFiles
        configure_file(${FILE} ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${FILE} COPYONLY)
        # Copy to final destination and set permissions
        file(COPY ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${FILE}
             DESTINATION ${CMAKE_BINARY_DIR}/test_conformance
             FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
             GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif(WIN32)
endforeach()
foreach(FILE test_conformance/${PY_FILES})

endforeach()
