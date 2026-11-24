#pragma once
#ifndef CATEGORY_RECORD_CODEC_H
#define CATEGORY_RECORD_CODEC_H

#include <string>
#include "ProductCategory.h"

class CategoryRecordCodec
{
public:
    static ProductCategory parse(const std::string& record);
    static std::string format(const ProductCategory& category);
};

#endif