add_executable(Clox
    main.c
    chunk.c
    memory.c
    debug.c
    value.c
)

target_include_directories(Clox
    PUBLIC "${PROJECT_SOURCE_DIR}/include"
    PUBLIC "${PROJECT_BINARY_DIR}"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

include(GenerateExportHeader)
generate_export_header(Clox
    EXPORT_MACRO_NAME API
    EXPORT_FILE_NAME "${PROJECT_BINARY_DIR}/CloxExport.h"
)
