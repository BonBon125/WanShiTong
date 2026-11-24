add_executable(roundtrip_test RoundTripTest.cpp)
target_link_libraries(roundtrip_test PRIVATE WanShiTong_core Qt6::Test)
add_test(NAME roundtrip COMMAND roundtrip_test)
set_tests_properties(roundtrip PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
