#pragma once
#ifndef BARCODE_DECODER_H
#define BARCODE_DECODER_H

#include <string>
#include <vector>
#include <stdexcept>
#include "barcodeEncoder.h" // reuses BarcodeCharacter
using namespace std;

class BarcodeDecoder
{
private:
    vector<BarcodeCharacter> characterTable;

    static const int ELEMENTS_PER_CHAR = 9;   // 9 elements per character
    static const int CHAR_COUNT = 8;   // '*' + 6 digits + '*'
    static const int GAP_COUNT = CHAR_COUNT - 1;
    static const int TOTAL_LENGTH = CHAR_COUNT * ELEMENTS_PER_CHAR + GAP_COUNT; // 79

    void initializeTable();
    char getCharForPattern(const string& pattern) const;

    // Decodes a barcode read left-to-right; throws on any defect.
    int decodeForward(const string& barcode) const;

public:
    BarcodeDecoder();

    // barcode: string of '0' (narrow) / '1' (wide), exactly as produced by
    // BarcodeEncoder::encode. Returns the 6-digit number (100000-999999).
    // Also accepts a barcode scanned backwards (Code 39 is bidirectional).
    // Throws invalid_argument on a malformed barcode,
    // out_of_range if the decoded number is not 6 digits.
    int decode(const string& barcode) const;
};

#endif