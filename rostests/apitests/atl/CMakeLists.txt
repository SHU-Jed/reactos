
set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

add_executable(atl_apitest
    CComBSTR.cpp
    CComHeapPtr.cpp
    testlist.c
    atl_apitest.rc)

target_link_libraries(atl_apitest wine uuid)
set_module_type(atl_apitest win32cui)
add_importlibs(atl_apitest ole32 oleaut32 user32 msvcrt kernel32)
add_cd_file(TARGET atl_apitest DESTINATION reactos/bin FOR all)
