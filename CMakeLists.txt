cmake_minimum_required (VERSION 2.8.12)
project (WST CXX C)

# The version number.
set (WST_VERSION_MAJOR 1)
set (WST_VERSION_MINOR 2)

#
# Boost 
#

# Utilib can make use of Boost signals
set(wst_boost_components "signals")

if(WIN32)
  set(Boost_USE_STATIC_LIBS TRUE)
endif()
# This is a workaround for the Boost probe since cmake 2.8.6
set(Boost_NO_BOOST_CMAKE TRUE)
# IBM xlC compiler may demand 1.47 or newer
find_package(Boost 1.37 REQUIRED COMPONENTS "${wst_boost_components}")

#
# BLAS
#

# Merlion can make use of the BLAS library if we can find it
include(FindBLAS)
set( HAVE_BLAS ${BLAS_FOUND} 
     CACHE BOOL "Enable interfaces to system BLAS" )
set( TEVA_SPOT_HAVE_BLAS ${BLAS_FOUND} 
     CACHE BOOL "Enable interfaces to system BLAS" )

#
# Java interfaces
#

# Theere are several libraries that provide interfaces to Java
find_package(JNI)
set( HAVE_JNI ${JNI_FOUND} 
     CACHE BOOL "Enable Java interfaces and wrappers" )

#
# Misc system headers and libraries
#

# Check for various header files and library functions
#   (LEGACY, mostly for GRASP)
include(CMakePushCheckState)
include(CheckFunctionExists)
include(CheckSymbolExists)
CHECK_INCLUDE_FILES(unistd.h TEVA_SPOT_HAVE_UNISTD_H)
CHECK_FUNCTION_EXISTS(srandom TEVA_SPOT_HAVE_SRANDOM)
cmake_push_check_state()
  set(CMAKE_REQUIRED_FLAGS -lm)
  CHECK_SYMBOL_EXISTS(log2 math.h TEVA_SPOT_HAVE_LOG2)
  CHECK_SYMBOL_EXISTS(rint math.h TEVA_SPOT_HAVE_RINT)
cmake_pop_check_state()

#
# Create and configure the teva_config.h header file
#

# Build the teva_config.h file and set the HAVE_CONFIG flag for all
# compile operations
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/teva_config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/teva_config.h
)
add_definitions(-DHAVE_CONFIG)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/teva_config.h DESTINATION include)


#
# Create the python virtual environment in the build tree.  This is
# needed for working with / testing pywst in the build tree.
#
#  TODO: Figure out how to "install" pywst...
#

# Create the Python virtual environment 
find_package(PythonInterp)
if (NOT PYTHONINTERP_FOUND)
  message(SEND_ERROR "Python interpreter required for PYWST / testing")
endif()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/python
         ${CMAKE_CURRENT_BINARY_DIR}/python.log
         ${CMAKE_CURRENT_BINARY_DIR}/python/bin/cxxtestgen
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/admin/vpy/python.zip
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tpl/vpykit/bin/install_python
          ${CMAKE_CURRENT_SOURCE_DIR}/admin/vpy/dev.ini
  COMMAND ${CMAKE_COMMAND}
  ARGS -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/python
  COMMAND ${PYTHON_EXECUTABLE}
  ARGS ${CMAKE_CURRENT_SOURCE_DIR}/tpl/vpykit/bin/install_python
       --src ${CMAKE_CURRENT_SOURCE_DIR}
       --build ${CMAKE_CURRENT_BINARY_DIR}
       --logfile ${CMAKE_CURRENT_BINARY_DIR}/python.log
       -c ${CMAKE_CURRENT_SOURCE_DIR}/admin/vpy/dev.ini
       -z ${CMAKE_CURRENT_SOURCE_DIR}/admin/vpy/python.zip
       -p %s/admin/wst_admin
       -p %s/tpl/acro/admin/acro-admin
       -p %s/tpl/acro/tpl/cxxtest/python
       -p %s/tpl/pyepanet
       -p %s/packages/pywst
)

add_custom_target( update-zip
  COMMAND ${PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tpl/vpykit/bin/create_zipfile
        -c ${CMAKE_CURRENT_SOURCE_DIR}/admin/vpy/dev.ini
        -d ${CMAKE_CURRENT_SOURCE_DIR}/admin/vpy
)

add_custom_target( update-zip-trunk
  COMMAND ${PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tpl/vpykit/bin/create_zipfile
        --trunk
        -c ${CMAKE_CURRENT_SOURCE_DIR}/admin/vpy/dev.ini
        -d ${CMAKE_CURRENT_SOURCE_DIR}/admin/vpy
)

add_custom_target(python-virtualenv ALL 
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/python)
add_custom_target(cxxtestgen
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/python/bin/cxxtestgen)

# Collect all compiled output in rational (easy-to-find) locations
set( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin )
set( LIBRARY_OUTPUT_PATH    ${PROJECT_BINARY_DIR}/lib )

# Compile things in tpl and packages
add_subdirectory(tpl)
add_subdirectory(packages)
