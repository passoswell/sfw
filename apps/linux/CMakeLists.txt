if(SFW_PLATFORM STREQUAL "Linux")

  message("Building executable for tests on linux, saving binary on ${CMAKE_BINARY_DIR}/bin/\r\n")

  add_executable(linux_tests
    main.cpp
  )

  sfw_configure_platform_executable(linux_tests)
  sfw_enable_platform_checks(linux_tests main.cpp)
endif()