file(GLOB_RECURSE SOURCES 
    "app_main/*.c"  # include all source files of app_main directory
    "communication/*.c"  # include all source files of communication directory
    "drivers/*.c"  # include all source files of drivers directory
    "Sources/*.c"   # include all source files of Sources directory
    "supervisor/*.c"  # include all source files of supervisor directory
    "tasks/*.c"  # include all source files of tasks directory
    )
                
idf_component_register(
                    SRCS ${SOURCES}
                    INCLUDE_DIRS "Include"  # include all header files of Include direcotry
                )