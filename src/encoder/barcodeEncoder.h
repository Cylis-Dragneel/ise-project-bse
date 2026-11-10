#ifndef BARCODE_ENCODER_H
#define BARCODE_ENCODER_H

#include <string>
#include <vector>
#include <stdexcept>
using namespace std;

class BarcodeCharacter
{
public:
    char symbol;
    string pattern; // 1 = wide Element , 0 = narrow Element

    BarcodeCharacter(char sym, const string &pat):symbol(sym), pattern(pat) {}
};

class BarcodeEncoder
{
private:
    vector<BarcodeCharacter> characterTable;

    void initializeTable();
    string getPatternForChar(char c) const;

public:
    BarcodeEncoder();

    //6-digit number (100000-999999)
    //return the encoder sequence of chars in the form of 0-1
    string encode(int number);
};

#endif