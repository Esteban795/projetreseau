set(
    AGENT_SOURCES 
    agent.c
)

add_library(agent STATIC ${AGENT_SOURCES})
target_include_directories(agent PUBLIC ${CMAKE_SOURCE_DIR}/include)