#include "barcodeEncoder.h"

BarcodeEncoder::BarcodeEncoder()
{
    initializeTable();
}

void BarcodeEncoder::initializeTable()
{
    //emplace used to call the BarcoderCharacter constructor
    //The Need 1-9 chars only
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

string BarcodeEncoder::getPatternForChar(char c) const
{
    for (int i=0; i<characterTable.size(); i++)
    {
        if (characterTable[i].symbol == c)
        {
            return characterTable[i].pattern;
        }
    }
    throw invalid_argument("Encoder Error: Invalid character encountered");
}

string BarcodeEncoder::encode(int number)
{
    //Given Requirement
    if (number < 100000 || number > 999999)
    {
        throw out_of_range("Encoder Error: Input must be a strictly 6-digit number between 100000 and 999999");
    }

    string payload = to_string(number);

    //Appending start and stop characters ('*')
    payload = "*" + payload + "*";

    string finalBarcodePattern = "";
    const string interCharGap = "0"; //Standard narrow space gap between characters

    //Traverse characters and insert inter-char gaps
    for (int i = 0; i < payload.length(); i++)
    {
        finalBarcodePattern += getPatternForChar(payload[i]);

        //final stop character
        if (i < payload.length() - 1)
        {
            finalBarcodePattern += interCharGap;
        }
    }

    return finalBarcodePattern;
}