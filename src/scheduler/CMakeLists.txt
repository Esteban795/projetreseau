set(
    SCHEDULER_SOURCES 
    scheduler.c
)

add_library(scheduler STATIC ${SCHEDULER_SOURCES})
target_include_directories(scheduler PUBLIC ${CMAKE_SOURCE_DIR}/include)