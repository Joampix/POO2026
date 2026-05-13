cmake_minimum_required(VERSION 3.21)

project(ConcaGymCpp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)

qt_add_executable(ConcaGymCpp
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/Models.h
    src/DataManager.h
    src/DataManager.cpp
    src/LocalAuditStore.h
    src/LocalAuditStore.cpp
    src/LoginDialog.h
    src/LoginDialog.cpp
    src/NutritionEngine.h
    src/NutritionEngine.cpp
    src/TrainingEngine.h
    src/TrainingEngine.cpp
    src/RecipeEngine.h
    src/RecipeEngine.cpp
    src/ExerciseDbClient.h
    src/ExerciseDbClient.cpp
    src/WgerClient.h
    src/WgerClient.cpp
    src/GeminiClient.h
    src/GeminiClient.cpp
    src/ProgressStore.h
    src/ProgressStore.cpp
)

target_link_libraries(ConcaGymCpp PRIVATE Qt6::Widgets Qt6::Network Qt6::Sql)

if(WIN32)
    set_target_properties(ConcaGymCpp PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
