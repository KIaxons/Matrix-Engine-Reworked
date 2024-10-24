# - Find DirectX SDK installation
# Find the DirectX 9 includes and library
# This module defines
#  DIRECTX9_INCLUDE_DIRS, where to find d3d9.h, etc.
#  DIRECTX9_LIBRARIES, libraries to link against to use DirectX.
#  DIRECTX9_FOUND, If false, do not try to use DirectX.
#  DIRECTX9_ROOT_DIR, directory where DirectX was installed.

# SDK download link:
# https://download.microsoft.com/download/A/E/7/AE743F1F-632B-4809-87A9-AA1BB3458E31/DXSDK_Jun10.exe

if(MSVC)
    set(DIRECTX9_INCLUDE_PATHS
        "$ENV{DXSDK_DIR}/Include"
        "$ENV{DIRECTX_ROOT}/Include"
        "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include"
        "C:/Program Files/Microsoft DirectX SDK (June 2010)/Include"
    )
    find_path(DIRECTX9_INCLUDE_DIRS d3dx9.h ${DIRECTX9_INCLUDE_PATHS} NO_DEFAULT_PATH)

    get_filename_component(DIRECTX9_ROOT_DIR "${DIRECTX9_INCLUDE_DIRS}/.." ABSOLUTE)

    set(DIRECTX9_LIBRARY_PATHS "${DIRECTX9_ROOT_DIR}/Lib/x86" "${DIRECTX9_ROOT_DIR}/Lib")

    find_library(DIRECTX9_D3D9_LIBRARY d3d9 ${DIRECTX9_LIBRARY_PATHS} NO_DEFAULT_PATH)
    find_library(DIRECTX9_D3DX9_LIBRARY d3dx9 ${DIRECTX9_LIBRARY_PATHS} NO_DEFAULT_PATH)
    find_library(DIRECTX9_DXERR_LIBRARY DxErr ${DIRECTX9_LIBRARY_PATHS} NO_DEFAULT_PATH)
    set(DIRECTX9_LIBRARIES ${DIRECTX9_D3D9_LIBRARY} ${DIRECTX9_D3DX9_LIBRARY} ${DIRECTX9_DXERR_LIBRARY})

    # handle the QUIETLY and REQUIRED arguments and set DIRECTX9_FOUND to TRUE if all listed variables are TRUE
    include(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(DIRECTX9 DEFAULT_MSG DIRECTX9_ROOT_DIR DIRECTX9_LIBRARIES DIRECTX9_INCLUDE_DIRS)
    mark_as_advanced(DIRECTX9_INCLUDE_DIRS DIRECTX9_D3D9_LIBRARY DIRECTX9_D3DX9_LIBRARY DIRECTX9_DXERR_LIBRARY)
else()
    message(STATUS "Skipping DirectX SDK search as the used compiler does not require it")
    set(DIRECTX9_LIBRARIES d3d9 d3dx9 dxerr9 d3dcompiler)
endif()