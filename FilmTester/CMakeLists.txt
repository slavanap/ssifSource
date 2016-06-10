project(FilmTester)

add_executable(${PROJECT_NAME} filmtester.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tools")
target_link_libraries(${PROJECT_NAME} avisynth.lib)

install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT Tools)
