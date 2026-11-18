#pragma once
#ifndef PRODUCT_FORMATTER_H
#define PRODUCT_FORMATTER_H

#include <string>
#include "Product.h"

// Turns a Product into text for display (the "Description" button / scan
// screen). Kept separate so the GUI, console or a report can reuse it.
class ProductFormatter
{
public:
    static std::string describe(const Product& product);
};

#endif