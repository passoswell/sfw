
# Add pcf8574 target
add_subdirectory(include/device/hd44780)

# Add all subdirectories under include/device
add_subdirectory(include/device/pcf8574)

# Collect all targets under include/device
add_library(device INTERFACE)
target_link_libraries(device INTERFACE hd44780 pcf8574)
