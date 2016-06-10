project(LoadHelper)

include_directories(${AVSTOOLS_INCLUDE_DIR})

add_library(${PROJECT_NAME} SHARED loadhelper.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tools")
target_link_libraries(${PROJECT_NAME} AvsTools)

install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT Tools)
