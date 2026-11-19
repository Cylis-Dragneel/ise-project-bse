#include "test_framework.h"

#include "barcodeEncoder.h"

#include <string>

namespace {

const std::string START_STOP_PATTERN = "010010100";

} // namespace

TEST_CASE(encoder_produces_expected_code39_pattern)
{
    BarcodeEncoder encoder;

    const std::string expected =
        "010010100"  // *
        "0" "100100001"  // 1
        "0" "001100001"  // 2
        "0" "101100000"  // 3
        "0" "000110001"  // 4
        "0" "100110000"  // 5
        "0" "001110000"  // 6
        "0" "010010100"; // *

    CHECK_EQ(encoder.encode(123456), expected);
}

TEST_CASE(encoder_output_is_79_binary_elements)
{
    BarcodeEncoder encoder;

    const std::string pattern = encoder.encode(123456);

    CHECK_EQ(pattern.size(), std::size_t{79});
    for (char element : pattern)
    {
        CHECK(element == '0' || element == '1');
    }
}

TEST_CASE(encoder_wraps_payload_in_start_and_stop_characters)
{
    BarcodeEncoder encoder;

    const std::string pattern = encoder.encode(654321);

    CHECK_EQ(pattern.substr(0, 9), START_STOP_PATTERN);
    CHECK_EQ(pattern.substr(pattern.size() - 9), START_STOP_PATTERN);
}

TEST_CASE(encoder_accepts_boundary_values)
{
    BarcodeEncoder encoder;

    CHECK_EQ(encoder.encode(100000).size(), std::size_t{79});
    CHECK_EQ(encoder.encode(999999).size(), std::size_t{79});
}

TEST_CASE(encoder_rejects_values_outside_six_digit_range)
{
    BarcodeEncoder encoder;

    CHECK_THROWS_AS(encoder.encode(0), std::out_of_range);
    CHECK_THROWS_AS(encoder.encode(-1), std::out_of_range);
    CHECK_THROWS_AS(encoder.encode(99999), std::out_of_range);
    CHECK_THROWS_AS(encoder.encode(1000000), std::out_of_range);
}
