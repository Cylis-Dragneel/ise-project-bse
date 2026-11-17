#include "OrderManager.h"
#include <fstream>
#include <iostream>
#include <ctime> //to get curr local time

using namespace std;

string OrderManager::generateOrderId(int dayOfMonth, const string &orderFilePath)
{
    time_t now = time(nullptr);
    tm *ltm = localtime(&now);

    int month = 1 + ltm->tm_mon;
    int day = (dayOfMonth > 0 && dayOfMonth <= 31) ? dayOfMonth : ltm->tm_mday;

    string monthStr = (month < 10 ? "0" : "") + to_string(month);
    string dayStr = (day < 10 ? "0" : "") + to_string(day);
    string dateSuffix = monthStr + dayStr; // MMDD

    // Scan Order.txt to count how many orders already exist for this specific date
    int dailyCount = 0;
    ifstream infile(orderFilePath);
    if (infile.is_open())
    {
        string line;
        while (getline(infile, line))
        {
            // Check if the line contains records for the same month and day
            if (line.find(dateSuffix) != string::npos)
            {
                dailyCount++;
            }
        }
        infile.close();
    }

    // Next sequence number for today
    dailyCount++;

    // Format the 4-digit counter (XXXX) with leading zeros
    string counterStr = to_string(dailyCount);
    while (counterStr.length() < 4)
    {
        counterStr = "0" + counterStr;
    }

    // Result: XXXXMMDD (4-digit counter + 2-digit month + 2-digit date)
    return counterStr + dateSuffix;
}

bool OrderManager::saveOrderTransaction(const Order &order,
                                        const vector<OrderDetail> &details,
                                        const string &orderFilePath,
                                        const string &detailFilePath)
{
    //Append to Order.txt
    ofstream orderFile(orderFilePath, ios::app);
    if (!orderFile.is_open())
    {
        cerr << "Error: Could not open " << orderFilePath << " for writing.\n";
        return false;
    }

    orderFile << order.orderId << " "
              << order.orderDate << " "
              << order.orderTime << " "
              << (order.customerName.empty() ? "Walk-in" : order.customerName) << " "
              << (order.customerContact.empty() ? "N/A" : order.customerContact) << " "
              << order.status << " "
              << order.paymentAmount << " "
              << order.paymentMode << " "
              << (order.paymentMerchant.empty() ? "None" : order.paymentMerchant)
              << " #\n";

    orderFile.close();

    //Write/Append to OrderDetail.txt
    ofstream detailFile(detailFilePath, ios::app);
    if (!detailFile.is_open())
    {
        cerr << "Error: Could not open " << detailFilePath << " for writing.\n";
        return false;
    }

    for (const auto &detail : details)
    {
        detailFile << detail.orderId << " "
                   << detail.productId << " "
                   << detail.productBarcode << " "
                   << detail.quantity
                   << " #\n";
    }

    detailFile.close();
    return true;
}