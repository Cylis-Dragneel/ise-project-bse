#include "barcodeDecoder.h"

BarcodeDecoder::BarcodeDecoder()
{
    initializeTable();
}

void BarcodeDecoder::initializeTable()
{
    // Must stay identical to the encoder's table
    characterTable.emplace_back('0', "000110100");
    characterTable.emplace_back('1', "100100001");
    characterTable.emplace_back('2', "001100001");
    characterTable.emplace_back('3', "101100000");
    characterTable.emplace_back('4', "000110001");
    characterTable.emplace_back('5', "100110000");
    characterTable.emplace_back('6', "001110000");
    characterTable.emplace_back('7', "000100101");
    characterTable.emplace_back('8', "100100100");
    characterTable.emplace_back('9', "001100100");
    characterTable.emplace_back('*', "010010100"); //Start/Stop character
}

char BarcodeDecoder::getCharForPattern(const string& pattern) const
{
    for (size_t i = 0; i < characterTable.size(); i++)
    {
        if (characterTable[i].pattern == pattern)
        {
            return characterTable[i].symbol;
        }
    }
    throw invalid_argument("Decoder Error: Unrecognized character pattern \"" + pattern + "\"");
}

int BarcodeDecoder::decodeForward(const string& barcode) const
{
    if (barcode.length() != (size_t)TOTAL_LENGTH)
    {
        throw invalid_argument("Decoder Error: Barcode must be exactly " +
            to_string(TOTAL_LENGTH) + " elements long (got " +
            to_string(barcode.length()) + ")");
    }

    for (size_t i = 0; i < barcode.length(); i++)
    {
        if (barcode[i] != '0' && barcode[i] != '1')
        {
            throw invalid_argument("Decoder Error: Barcode may contain only '0' and '1'");
        }
    }

    string decoded = "";
    for (int c = 0; c < CHAR_COUNT; c++)
    {
        int start = c * (ELEMENTS_PER_CHAR + 1);

        // Every character except the last must be followed by a narrow gap
        if (c < CHAR_COUNT - 1 && barcode[start + ELEMENTS_PER_CHAR] != '0')
        {
            throw invalid_argument("Decoder Error: Invalid inter-character gap after character " +
                to_string(c + 1));
        }

        decoded += getCharForPattern(barcode.substr(start, ELEMENTS_PER_CHAR));
    }

    // Start and stop markers
    if (decoded[0] != '*' || decoded[CHAR_COUNT - 1] != '*')
    {
        throw invalid_argument("Decoder Error: Missing start/stop character");
    }

    string digits = decoded.substr(1, CHAR_COUNT - 2);

    // '*' is not allowed inside the payload
    if (digits.find('*') != string::npos)
    {
        throw invalid_argument("Decoder Error: Start/stop character found inside data");
    }

    int number = stoi(digits);
    if (number < 100000 || number > 999999)
    {
        throw out_of_range("Decoder Error: Decoded value is not a 6-digit number between 100000 and 999999");
    }
    return number;
}

int BarcodeDecoder::decode(const string& barcode) const
{
    try
    {
        return decodeForward(barcode);
    }
    catch (const exception&)
    {
        // Try the barcode as if it was scanned backwards
        string reversed(barcode.rbegin(), barcode.rend());
        try
        {
            return decodeForward(reversed);
        }
        catch (const exception&)
        {
            // Neither direction worked: report the error for the barcode as given
            return decodeForward(barcode);
        }
    }
}