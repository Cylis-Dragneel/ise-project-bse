#include "OrderManager.h"

#include <cerrno>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string sanitize(std::string text)
{
    if (text.empty())
    {
        return "-";
    }

    for (char &character : text)
    {
        const unsigned char value = static_cast<unsigned char>(character);
        if (character == ' ' || character == '\t' || character == '#' ||
            value < 0x20 || value == 0x7F)
        {
            character = '_';
        }
    }

    return text;
}

std::string desanitize(std::string text)
{
    if (text == "-")
    {
        return "";
    }

    for (char &character : text)
    {
        if (character == '_')
        {
            character = ' ';
        }
    }

    return text;
}

std::vector<std::string> split(const std::string &line)
{
    std::vector<std::string> tokens;
    std::size_t position = 0;

    while (position < line.size())
    {
        while (position < line.size() &&
               (line[position] == ' ' || line[position] == '\t' ||
                line[position] == '\r'))
        {
            ++position;
        }

        const std::size_t start = position;
        while (position < line.size() && line[position] != ' ' &&
               line[position] != '\t' && line[position] != '\r')
        {
            ++position;
        }

        if (position > start)
        {
            tokens.push_back(line.substr(start, position - start));
        }
    }

    return tokens;
}

bool isDigits(const std::string &text)
{
    if (text.empty())
    {
        return false;
    }

    for (char character : text)
    {
        if (character < '0' || character > '9')
        {
            return false;
        }
    }

    return true;
}

bool isBarcode(const std::string &barcode)
{
    // Product barcodes are 6-digit Code 39 values in 100000-999999.
    return barcode.size() == 6 && isDigits(barcode) && barcode[0] != '0';
}

bool isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int daysInMonth(int month, int year)
{
    static const int days[] = {31, 28, 31, 30, 31, 30,
                               31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year))
    {
        return 29;
    }
    return days[month - 1];
}

bool validDate(const std::string &date)
{
    if (date.size() != 10 || date[4] != '-' || date[7] != '-' ||
        !isDigits(date.substr(0, 4)) || !isDigits(date.substr(5, 2)) ||
        !isDigits(date.substr(8, 2)))
    {
        return false;
    }

    const int year = std::atoi(date.substr(0, 4).c_str());
    const int month = std::atoi(date.substr(5, 2).c_str());
    const int day = std::atoi(date.substr(8, 2).c_str());

    if (month < 1 || month > 12 || day < 1)
    {
        return false;
    }

    return day <= daysInMonth(month, year);
}

bool validTime(const std::string &time)
{
    if (time.size() != 8 || time[2] != ':' || time[5] != ':' ||
        !isDigits(time.substr(0, 2)) || !isDigits(time.substr(3, 2)) ||
        !isDigits(time.substr(6, 2)))
    {
        return false;
    }

    const int hour = std::atoi(time.substr(0, 2).c_str());
    const int minute = std::atoi(time.substr(3, 2).c_str());
    const int second = std::atoi(time.substr(6, 2).c_str());

    return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 &&
           second >= 0 && second <= 59;
}

std::string mmdd(const std::string &date)
{
    return date.substr(5, 2) + date.substr(8, 2);
}

bool isValidStatus(const std::string &status)
{
    return status == "in-progress" || status == "on-hold" ||
           status == "paid" || status == "archived";
}

bool isValidPaymentMode(const std::string &mode)
{
    return mode == "cash" || mode == "card" || mode == "wallet";
}

std::string formatNumber(double value)
{
    if (value == 0.0)
    {
        return "0";
    }

    std::ostringstream output;
    output << std::fixed << std::setprecision(6) << value;
    std::string text = output.str();

    const std::size_t dot = text.find('.');
    if (dot != std::string::npos)
    {
        while (!text.empty() && text.back() == '0')
        {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.')
        {
            text.pop_back();
        }
    }

    return text;
}

bool parseDouble(const std::string &text, double &value)
{
    if (text.empty() || text.find_first_of("eE") != std::string::npos)
    {
        return false;
    }

    char *end = nullptr;
    errno = 0;
    const double parsed = std::strtod(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0' || errno == ERANGE ||
        !std::isfinite(parsed))
    {
        return false;
    }

    value = parsed;
    return true;
}

bool parseInt(const std::string &text, int &value)
{
    if (text.empty())
    {
        return false;
    }

    char *end = nullptr;
    errno = 0;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0' || errno == ERANGE ||
        parsed < INT_MIN || parsed > INT_MAX)
    {
        return false;
    }

    value = static_cast<int>(parsed);
    return true;
}

bool orderIdExists(const std::string &orderId, const std::string &path)
{
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line))
    {
        const std::vector<std::string> tokens = split(line);
        if (!tokens.empty() && tokens[0] == orderId)
        {
            return true;
        }
    }

    return false;
}

std::string formatOrderLine(const Order &order)
{
    std::ostringstream line;
    line << order.orderId << ' ' << order.date << ' ' << order.time << ' '
         << sanitize(order.customerName) << ' '
         << sanitize(order.contact) << ' ' << order.status << ' '
         << formatNumber(order.paymentAmount) << ' '
         << order.paymentMode << ' ' << sanitize(order.merchant) << " #";
    return line.str();
}

std::string formatDetailLine(const OrderDetail &detail)
{
    std::ostringstream line;
    line << detail.orderId << ' ' << detail.productId << ' '
         << detail.productBarcode << ' ' << formatNumber(detail.quantity)
         << " #";
    return line.str();
}

bool readLines(const std::string &path, std::vector<std::string> &lines)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    lines.clear();
    std::string line;
    while (std::getline(file, line))
    {
        lines.push_back(line);
    }

    return !file.bad();
}

// Rebuilds `lines`, putting `newLines` where the first row of `targetId`
// was (so file order is preserved) and dropping any other rows of that id.
// When the id has no rows yet, `newLines` are appended. `found` reports
// whether at least one existing row matched.
std::vector<std::string> replaceRows(const std::vector<std::string> &lines,
                                     const std::string &targetId,
                                     const std::vector<std::string> &newLines,
                                     bool &found)
{
    std::vector<std::string> result;
    bool inserted = false;
    found = false;

    for (const std::string &line : lines)
    {
        const std::vector<std::string> tokens = split(line);
        if (!tokens.empty() && tokens[0] == targetId)
        {
            found = true;
            if (!inserted)
            {
                result.insert(result.end(), newLines.begin(), newLines.end());
                inserted = true;
            }
            continue;
        }
        result.push_back(line);
    }

    if (!inserted)
    {
        result.insert(result.end(), newLines.begin(), newLines.end());
    }

    return result;
}

// Writes `lines` to `tempPath` and flushes it. Does not touch the real file.
bool writeTempFile(const std::string &tempPath,
                   const std::vector<std::string> &lines)
{
    std::ofstream out(tempPath, std::ios::trunc);
    if (!out.is_open())
    {
        return false;
    }

    for (const std::string &line : lines)
    {
        out << line << '\n';
    }

    out.flush();
    const bool ok = out.good();
    out.close();
    return ok;
}

// Atomically swaps the finished temp file over the real one.
// std::filesystem::rename replaces an existing target on POSIX and Windows.
bool commitTempFile(const std::string &tempPath, const std::string &path)
{
    std::error_code error;
    std::filesystem::rename(tempPath, path, error);
    if (error)
    {
        std::filesystem::remove(tempPath, error);
        return false;
    }
    return true;
}

void discardTempFile(const std::string &tempPath)
{
    std::error_code error;
    std::filesystem::remove(tempPath, error);
}

} // namespace

std::string OrderManager::getCurrentDate()
{
    const std::time_t now = std::time(nullptr);
    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&now));
    return buffer;
}

std::string OrderManager::getCurrentTime()
{
    const std::time_t now = std::time(nullptr);
    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&now));
    return buffer;
}

std::string OrderManager::generateOrderId(const std::string &dateIn,
                                          const std::string &orderFilePath)
{
    const std::string date = dateIn.empty() ? getCurrentDate() : dateIn;
    if (!validDate(date))
    {
        return "";
    }

    int maxSequence = 0;
    std::ifstream file(orderFilePath);
    std::string line;

    while (std::getline(file, line))
    {
        const std::vector<std::string> tokens = split(line);
        if (tokens.size() < 2)
        {
            continue;
        }

        // Use the date field, not a substring match on the whole line.
        if (tokens[1] != date)
        {
            continue;
        }

        if (tokens[0].size() != 8 || !isDigits(tokens[0]))
        {
            continue;
        }

        const int sequence = std::atoi(tokens[0].substr(0, 4).c_str());
        if (sequence > maxSequence)
        {
            maxSequence = sequence;
        }
    }

    if (maxSequence >= 9999)
    {
        return "";
    }

    std::string sequence = std::to_string(maxSequence + 1);
    while (sequence.size() < 4)
    {
        sequence = "0" + sequence;
    }

    return sequence + mmdd(date);
}

bool OrderManager::validateOrder(const Order &order,
                                 const std::vector<OrderDetail> &details)
{
    if (order.orderId.size() != 8 || !isDigits(order.orderId) ||
        order.orderId.substr(0, 4) == "0000")
    {
        return false;
    }

    if (!validDate(order.date) || !validTime(order.time) ||
        order.orderId.substr(4) != mmdd(order.date))
    {
        return false;
    }

    if (!isValidStatus(order.status) ||
        !isValidPaymentMode(order.paymentMode))
    {
        return false;
    }

    if (!std::isfinite(order.paymentAmount) || order.paymentAmount < 0)
    {
        return false;
    }

    if (details.empty())
    {
        return false;
    }

    for (const OrderDetail &detail : details)
    {
        if (detail.orderId != order.orderId || detail.productId <= 0 ||
            !isBarcode(detail.productBarcode) ||
            !std::isfinite(detail.quantity) || detail.quantity <= 0)
        {
            return false;
        }
    }

    return true;
}

bool OrderManager::saveOrderTransaction(
    const Order &order,
    const std::vector<OrderDetail> &details,
    const std::string &orderFilePath,
    const std::string &detailFilePath)
{
    if (!validateOrder(order, details))
    {
        return false;
    }

    // A caller-supplied id must not create a second order record.
    if (orderIdExists(order.orderId, orderFilePath))
    {
        return false;
    }

    // Open both files before writing so an invalid path cannot leave a
    // partial order behind.
    std::ofstream detailFile(detailFilePath, std::ios::app);
    std::ofstream orderFile(orderFilePath, std::ios::app);
    if (!detailFile.is_open() || !orderFile.is_open())
    {
        return false;
    }

    for (const OrderDetail &detail : details)
    {
        detailFile << formatDetailLine(detail) << '\n';
    }

    if (!detailFile.good())
    {
        return false;
    }

    orderFile << formatOrderLine(order) << '\n';

    detailFile.flush();
    orderFile.flush();

    return detailFile.good() && orderFile.good();
}

std::vector<Order> OrderManager::loadOrders(const std::string &orderFilePath)
{
    std::vector<Order> orders;
    std::ifstream file(orderFilePath);
    std::string line;

    while (std::getline(file, line))
    {
        const std::vector<std::string> tokens = split(line);
        if (tokens.size() != 10 || tokens[9] != "#")
        {
            continue;
        }

        if (tokens[0].size() != 8 || !isDigits(tokens[0]) ||
            tokens[0].substr(0, 4) == "0000")
        {
            continue;
        }

        if (!validDate(tokens[1]) || !validTime(tokens[2]) ||
            tokens[0].substr(4) != mmdd(tokens[1]))
        {
            continue;
        }

        if (!isValidStatus(tokens[5]) || !isValidPaymentMode(tokens[7]))
        {
            continue;
        }

        double paymentAmount = 0.0;
        if (!parseDouble(tokens[6], paymentAmount) || paymentAmount < 0)
        {
            continue;
        }

        Order order;
        order.orderId = tokens[0];
        order.date = tokens[1];
        order.time = tokens[2];
        order.customerName = desanitize(tokens[3]);
        order.contact = desanitize(tokens[4]);
        order.status = tokens[5];
        order.paymentAmount = paymentAmount;
        order.paymentMode = tokens[7];
        order.merchant = desanitize(tokens[8]);
        orders.push_back(order);
    }

    return orders;
}

std::vector<OrderDetail> OrderManager::loadOrderDetails(
    const std::string &orderId,
    const std::string &detailFilePath)
{
    std::vector<OrderDetail> details;
    std::ifstream file(detailFilePath);
    std::string line;

    while (std::getline(file, line))
    {
        const std::vector<std::string> tokens = split(line);
        if (tokens.size() != 5 || tokens[4] != "#" ||
            tokens[0] != orderId)
        {
            continue;
        }

        int productId = 0;
        double quantity = 0.0;
        if (!parseInt(tokens[1], productId) || productId <= 0 ||
            !isBarcode(tokens[2]) ||
            !parseDouble(tokens[3], quantity) || quantity <= 0)
        {
            continue;
        }

        OrderDetail detail;
        detail.orderId = tokens[0];
        detail.productId = productId;
        detail.productBarcode = tokens[2];
        detail.quantity = quantity;
        details.push_back(detail);
    }

    return details;
}

bool OrderManager::findOrder(const std::string &orderId,
                             Order &result,
                             const std::string &orderFilePath)
{
    const std::vector<Order> orders = loadOrders(orderFilePath);

    for (const Order &order : orders)
    {
        if (order.orderId == orderId)
        {
            result = order;
            return true;
        }
    }

    return false;
}

bool OrderManager::updateOrderTransaction(
    const Order &order,
    const std::vector<OrderDetail> &details,
    const std::string &orderFilePath,
    const std::string &detailFilePath)
{
    if (!validateOrder(order, details))
    {
        return false;
    }

    // Read everything first: nothing is modified if either file is unusable.
    std::vector<std::string> orderLines;
    std::vector<std::string> detailLines;
    if (!readLines(orderFilePath, orderLines) ||
        !readLines(detailFilePath, detailLines))
    {
        return false;
    }

    std::vector<std::string> newDetailLines;
    for (const OrderDetail &detail : details)
    {
        newDetailLines.push_back(formatDetailLine(detail));
    }

    bool orderFound = false;
    bool detailsFound = false;
    const std::vector<std::string> newOrderContent = replaceRows(
        orderLines, order.orderId, {formatOrderLine(order)}, orderFound);
    if (!orderFound)
    {
        return false; // unknown order id: updates never create orders
    }
    const std::vector<std::string> newDetailContent = replaceRows(
        detailLines, order.orderId, newDetailLines, detailsFound);

    // Stage both new files completely before replacing either original.
    const std::string orderTemp = orderFilePath + ".tmp";
    const std::string detailTemp = detailFilePath + ".tmp";

    if (!writeTempFile(orderTemp, newOrderContent) ||
        !writeTempFile(detailTemp, newDetailContent))
    {
        discardTempFile(orderTemp);
        discardTempFile(detailTemp);
        return false;
    }

    if (!commitTempFile(detailTemp, detailFilePath))
    {
        discardTempFile(orderTemp);
        return false;
    }

    if (!commitTempFile(orderTemp, orderFilePath))
    {
        // Put the original detail rows back so the two files still agree.
        if (writeTempFile(detailTemp, detailLines))
        {
            commitTempFile(detailTemp, detailFilePath);
        }
        else
        {
            discardTempFile(detailTemp);
        }
        return false;
    }

    return true;
}

// Loads only the orders whose status equals statusFilter ("" = all).
std::vector<Order> OrderManager::loadOrdersByStatus(
    const std::string &statusFilter,
    const std::string &orderFilePath)
{
    const std::vector<Order> allOrders = loadOrders(orderFilePath);
    if (statusFilter.empty())
    {
        return allOrders;
    }

    std::vector<Order> filtered;
    for (const Order &order : allOrders)
    {
        if (order.status == statusFilter)
        {
            filtered.push_back(order);
        }
    }
    return filtered;
}