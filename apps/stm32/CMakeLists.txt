if(SFW_PLATFORM STREQUAL "STM32")

  message("Building executable for tests on stm32, saving binary on ${CMAKE_BINARY_DIR}/bin/\r\n")

  add_executable(stm32_tests
    main.cpp
  )

  sfw_configure_platform_executable(stm32_tests)
  sfw_enable_platform_checks(
    stm32_tests
    main.cpp)
endif()