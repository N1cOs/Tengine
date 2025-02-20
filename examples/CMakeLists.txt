link_directories(${TENGINE_COMMON_LIB_DIRS})

# add examples with metal
# macro for adding test
macro (tengine_example name file)
    add_executable(${name}
        "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
        "${CMAKE_CURRENT_SOURCE_DIR}/common/tengine_operations.c")

    target_link_libraries(${name} PRIVATE ${CMAKE_PROJECT_NAME} m)
    include_directories("${PROJECT_SOURCE_DIR}/examples/common")
    install (TARGETS ${name} DESTINATION bin)
endmacro()

# add c++ api examples
if (TENGINE_BUILD_CPP_API)
    tengine_example(cpp_tm_classification       cpp_tm_classification.cpp)
    tengine_example(cpp_tm_mobilenet_ssd        cpp_tm_mobilenet_ssd.cpp)
endif()

# add c api examples
tengine_example(tm_classification           tm_classification.c)
tengine_example(tm_classification_fp16      tm_classification_fp16.c)
tengine_example(tm_classification_int8      tm_classification_int8.c)
tengine_example(tm_classification_uint8     tm_classification_uint8.c)
tengine_example(tm_classification_vulkan    tm_classification_vulkan.c)
tengine_example(tm_classification_acl       tm_classification_acl.c)
tengine_example(tm_classification_timvx     tm_classification_timvx.c)
tengine_example(tm_classification_trt       tm_classification_trt.cpp)
tengine_example(tm_classification_cuda      tm_classification_cuda.cpp)
tengine_example(tm_mobilenet_ssd            tm_mobilenet_ssd.c)
tengine_example(tm_mobilenet_ssd_uint8      tm_mobilenet_ssd_uint8.cpp)
tengine_example(tm_mobilenet_ssd_acl        tm_mobilenet_ssd_acl.c)
tengine_example(tm_retinaface               tm_retinaface.cpp)
tengine_example(tm_yolov3_tiny              tm_yolov3_tiny.cpp)
tengine_example(tm_yolov3_uint8             tm_yolov3_uint8.cpp)
tengine_example(tm_landmark                 tm_landmark.cpp)
tengine_example(tm_landmark_uint8           tm_landmark_uint8.cpp)
tengine_example(tm_mobilefacenet            tm_mobilefacenet.cpp)
tengine_example(tm_yolov4                   tm_yolov4.cpp)
tengine_example(tm_yolov5                   tm_yolov5.cpp)
tengine_example(tm_yolofastest              tm_yolofastest.cpp)
tengine_example(tm_ultraface                tm_ultraface.cpp)


# add examples with opencv
# set(OpenCV_DIR /mnt/d/ubuntu/opencv_install/linux-armv7/lib/cmake/opencv4)
find_package(OpenCV QUIET)

if(OpenCV_FOUND AND ${TENGINE_TARGET_PROCESSOR} MATCHES "X86")
include_directories(${OpenCV_INCLUDE_DIRS})

    # macro for adding examples
    macro (tengine_example_cv name file)
        add_executable(${name}
                "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
                "${CMAKE_CURRENT_SOURCE_DIR}/common/tengine_operations.c")
        target_link_libraries(${name} ${CMAKE_PROJECT_NAME} ${OpenCV_LIBS})
        install (TARGETS ${name} DESTINATION bin)
    endmacro()
    tengine_example_cv(tm_openpose              tm_openpose.cpp)
    tengine_example_cv(tm_yolact                tm_yolact.cpp)
    tengine_example_cv(tm_crnn                  tm_crnn.cpp)
    tengine_example_cv(tm_alphapose             tm_alphapose.cpp)
    tengine_example_cv(tm_yolov4_tiny           tm_yolov4_tiny.cpp)
    tengine_example_cv(tm_yolov4_tiny_timvx     tm_yolov4_tiny_timvx.cpp)    
    tengine_example_cv(tm_yolov5s               tm_yolov5s.cpp)
    tengine_example_cv(tm_yolov5s_timvx         tm_yolov5s_timvx.cpp)
else()
    message(WARNING "OpenCV not found, some examples won't be built")
endif()

if (TENGINE_ENABLE_NNIE)
    function (tengine_nnie_example name file)
        add_executable(${name} 
                    "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/common/tengine_operations.c")    

        target_link_libraries(${name} PRIVATE ${CMAKE_PROJECT_NAME})

        include_directories("${PROJECT_SOURCE_DIR}/src/dev/nnie/include")
        include_directories("${PROJECT_SOURCE_DIR}/src/dev/nnie/common")
        include_directories("${PROJECT_SOURCE_DIR}/3rdparty/nnie/include")

        install (TARGETS ${name} DESTINATION bin)
    endfunction()

    # add c api examples
    tengine_nnie_example(tm_yolov3_nnie             tm_yolov3_nnie.cpp)
endif()
