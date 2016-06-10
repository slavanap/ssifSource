project(TinyXML2)
add_library(${PROJECT_NAME} STATIC tinyxml2.h tinyxml2.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Third Party")
