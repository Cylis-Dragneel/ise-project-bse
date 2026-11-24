#pragma once
#ifndef STOCKCHANGE_H
#define STOCKCHANGE_H

#include <string>

struct StockChange {
    std::string barcode;
    double delta;
};

#endif