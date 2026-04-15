file(GLOB_RECURSE SOURCES 
    "Sources/*.c"   # include all source files of Sources directory
    "app_main/*.c"  # include all source files of app_main directory
    )
                
idf_component_register(
                    SRCS ${SOURCES}
                    INCLUDE_DIRS "Include"  # include all header files of Include direcotry
                )