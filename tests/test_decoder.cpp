#include "test_framework.h"

#include "barcodeDecoder.h"
#include "barcodeEncoder.h"

#include <string>

namespace {

const std::string START_STOP_PATTERN = "010010100";
const std::string ZERO_PATTERN = "000110100";

std::string reversed(std::string value)
{
    return std::string(value.rbegin(), value.rend());
}

} // namespace

TEST_CASE(decoder_round_trips_representative_values)
{
    BarcodeEncoder encoder;
    BarcodeDecoder decoder;

    const int values[] = {100000, 123456, 654321, 999999};
    for (int value : values)
    {
        CHECK_EQ(decoder.decode(encoder.encode(value)), value);
    }
}

TEST_CASE(decoder_accepts_barcodes_scanned_backwards)
{
    BarcodeEncoder encoder;
    BarcodeDecoder decoder;

    const int values[] = {100000, 123456, 654321, 999999};
    for (int value : values)
    {
        CHECK_EQ(decoder.decode(reversed(encoder.encode(value))), value);
    }
}

TEST_CASE(decoder_round_trips_every_six_digit_value)
{
    BarcodeEncoder encoder;
    BarcodeDecoder decoder;

    for (int value = 100000; value <= 999999; ++value)
    {
        const std::string pattern = encoder.encode(value);

        CHECK_EQ(decoder.decode(pattern), value);
        CHECK_EQ(decoder.decode(reversed(pattern)), value);
    }
}

TEST_CASE(decoder_rejects_wrong_length)
{
    BarcodeDecoder decoder;

    CHECK_THROWS_AS(decoder.decode(""), std::invalid_argument);
    CHECK_THROWS_AS(decoder.decode("010101"), std::invalid_argument);
    CHECK_THROWS_AS(decoder.decode(std::string(78, '0')), std::invalid_argument);
    CHECK_THROWS_AS(decoder.decode(std::string(80, '0')), std::invalid_argument);
}

TEST_CASE(decoder_rejects_non_binary_elements)
{
    BarcodeEncoder encoder;
    BarcodeDecoder decoder;

    std::string pattern = encoder.encode(123456);
    pattern[0] = '2';

    CHECK_THROWS_AS(decoder.decode(pattern), std::invalid_argument);
}

TEST_CASE(decoder_rejects_invalid_inter_character_gap)
{
    BarcodeEncoder encoder;
    BarcodeDecoder decoder;

    std::string pattern = encoder.encode(123456);
    pattern[9] = '1';

    CHECK_THROWS_AS(decoder.decode(pattern), std::invalid_argument);
}

TEST_CASE(decoder_rejects_missing_start_character)
{
    BarcodeEncoder encoder;
    BarcodeDecoder decoder;

    std::string pattern = encoder.encode(123456);
    pattern.replace(0, 9, ZERO_PATTERN);

    CHECK_THROWS_AS(decoder.decode(pattern), std::invalid_argument);
}

TEST_CASE(decoder_rejects_missing_stop_character)
{
    BarcodeEncoder encoder;
    BarcodeDecoder decoder;

    std::string pattern = encoder.encode(123456);
    pattern.replace(pattern.size() - 9, 9, ZERO_PATTERN);

    CHECK_THROWS_AS(decoder.decode(pattern), std::invalid_argument);
}

TEST_CASE(decoder_rejects_start_stop_character_inside_payload)
{
    BarcodeEncoder encoder;
    BarcodeDecoder decoder;

    std::string pattern = encoder.encode(123456);
    pattern.replace(10, 9, START_STOP_PATTERN);

    CHECK_THROWS_AS(decoder.decode(pattern), std::invalid_argument);
}

TEST_CASE(decoder_rejects_unrecognized_character_pattern)
{
    BarcodeEncoder encoder;
    BarcodeDecoder decoder;

    std::string pattern = encoder.encode(123456);
    pattern.replace(10, 9, "111111111");

    CHECK_THROWS_AS(decoder.decode(pattern), std::invalid_argument);
}

TEST_CASE(decoder_rejects_payload_outside_six_digit_range)
{
    BarcodeDecoder decoder;

    // Build a structurally valid barcode whose payload is "000000".
    std::string pattern = START_STOP_PATTERN;
    for (int digit = 0; digit < 6; ++digit)
    {
        pattern += "0";
        pattern += ZERO_PATTERN;
    }
    pattern += "0";
    pattern += START_STOP_PATTERN;

    CHECK_EQ(pattern.size(), std::size_t{79});
    CHECK_THROWS_AS(decoder.decode(pattern), std::out_of_range);
}
