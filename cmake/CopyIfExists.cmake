if(DEFINED SRC AND DEFINED DST AND EXISTS "${SRC}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SRC}" "${DST}")
endif()

