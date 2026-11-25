#include "ReportsService.h"

#include "billing/BillCalculator.h"
#include "billing/FileBillDetailRepository.h"
#include "billing/FileBillRepository.h"
#include "OrderManager.h"
#include "catalog/ProductCatalog.h"
#include "seasons/ISeasonProvider.h"
#include "util/DateUtils.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <unordered_map>
#include <vector>

using std::string;
using std::vector;

namespace
{
    bool parseDate(const string &text, int &year, int &month, int &day)
    {
        if (!DateUtils::isValidDate(text))
            return false;

        year = (text[0] - '0') * 1000 + (text[1] - '0') * 100 +
               (text[2] - '0') * 10 + (text[3] - '0');
        month = (text[5] - '0') * 10 + (text[6] - '0');
        day = (text[8] - '0') * 10 + (text[9] - '0');
        return true;
    }

    // Howard Hinnant's civil-date algorithms: deterministic, timezone-free.
    long long daysFromCivil(int year, unsigned month, unsigned day)
    {
        year -= month <= 2;
        const long long era = (year >= 0 ? year : year - 399) / 400;
        const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
        const unsigned dayOfYear =
            (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
        const unsigned dayOfEra =
            yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
        return era * 146097 + static_cast<long long>(dayOfEra) - 719468;
    }

    void civilFromDays(long long days, int &year, int &month, int &day)
    {
        days += 719468;
        const long long era = (days >= 0 ? days : days - 146096) / 146097;
        const unsigned dayOfEra = static_cast<unsigned>(days - era * 146097);
        const unsigned yearOfEra =
            (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 -
             dayOfEra / 146096) / 365;
        year = static_cast<int>(yearOfEra) + static_cast<int>(era) * 400;
        const unsigned dayOfYear =
            dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
        const unsigned monthPart = (5 * dayOfYear + 2) / 153;
        day = static_cast<int>(dayOfYear - (153 * monthPart + 2) / 5 + 1);
        month = static_cast<int>(monthPart + (monthPart < 10 ? 3 : -9));
        year += (month <= 2);
    }

    string formatDate(int year, int month, int day)
    {
        char buffer[11];
        std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", year, month,
                      day);
        return string(buffer);
    }

    string addDays(const string &date, long long amount)
    {
        int year = 0;
        int month = 0;
        int day = 0;
        parseDate(date, year, month, day);
        civilFromDays(daysFromCivil(year, static_cast<unsigned>(month),
                                    static_cast<unsigned>(day)) +
                          amount,
                      year, month, day);
        return formatDate(year, month, day);
    }

    // 0 = Monday ... 6 = Sunday.
    int weekdayMonday(const string &date)
    {
        int year = 0;
        int month = 0;
        int day = 0;
        parseDate(date, year, month, day);
        const long long days = daysFromCivil(
            year, static_cast<unsigned>(month), static_cast<unsigned>(day));
        return static_cast<int>((days + 3) % 7 + 7) % 7;
    }

    string weekStart(const string &date)
    {
        return addDays(date, -weekdayMonday(date));
    }

    bool isLeapYear(int year)
    {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    int daysInMonth(int year, int month)
    {
        static const int days[] = {31, 28, 31, 30, 31, 30,
                                   31, 31, 30, 31, 30, 31};
        if (month == 2 && isLeapYear(year))
            return 29;
        return days[month - 1];
    }

    bool isCompleted(const string &status)
    {
        return status == "paid" || status == "archived";
    }

    double specialDiscountAmount(const Bill &bill)
    {
        const double base = BillCalculator::roundMoney(
            bill.totalNoTax + bill.totalTax - bill.totalDiscount);
        const double afterSeasonal =
            BillCalculator::roundMoney(base - bill.seasonalDiscount);
        return BillCalculator::roundMoney(
            afterSeasonal * bill.specialDiscountPercent / 100.0);
    }
} // namespace

namespace reports
{
    ReportsService::ReportsService(const ProductCatalog &catalog,
                                   string orderPath,
                                   string orderDetailPath,
                                   string billPath,
                                   string billDetailPath,
                                   const ISeasonProvider *seasons)
        : catalog_(catalog),
          orderPath_(std::move(orderPath)),
          orderDetailPath_(std::move(orderDetailPath)),
          billPath_(std::move(billPath)),
          billDetailPath_(std::move(billDetailPath)),
          seasons_(seasons)
    {
    }

    vector<Bill> ReportsService::loadBills() const
    {
        FileBillRepository billRepository(billPath_);
        FileBillDetailRepository detailRepository(billDetailPath_);

        vector<Bill> bills = billRepository.loadAll();
        const vector<BillLine> lines = detailRepository.loadAll();

        std::unordered_map<string, vector<BillLine>> linesByBill;
        for (const BillLine &line : lines)
        {
            linesByBill[line.billId].push_back(line);
        }

        for (Bill &bill : bills)
        {
            const auto found = linesByBill.find(bill.billId);
            if (found != linesByBill.end())
            {
                bill.lines = found->second;
            }
        }

        return bills;
    }

    double ReportsService::quoteOrder(
        const Order &order,
        const vector<OrderDetail> &details) const
    {
        double subtotal = 0;
        double tax = 0;
        double discount = 0;

        for (const OrderDetail &detail : details)
        {
            const Product *product = catalog_.findByBarcode(detail.productBarcode);
            if (product == nullptr)
            {
                product = catalog_.findById(detail.productId);
            }
            if (product == nullptr)
            {
                continue;
            }

            subtotal += BillCalculator::roundMoney(product->unitPrice() *
                                                   detail.quantity);
            tax += BillCalculator::roundMoney(product->taxAmount() *
                                              detail.quantity);
            discount += BillCalculator::roundMoney(product->discountPerUnit() *
                                                   detail.quantity);
        }

        const double base =
            BillCalculator::roundMoney(subtotal + tax - discount);

        double seasonalPercent = 0;
        if (seasons_ != nullptr)
        {
            try
            {
                seasonalPercent = seasons_->activePercent(order.date);
            }
            catch (...)
            {
                seasonalPercent = 0;
            }
        }

        const double seasonal = BillCalculator::roundMoney(
            base * seasonalPercent / 100.0);
        return BillCalculator::roundMoney(base - seasonal);
    }

    vector<OrderRow> ReportsService::incompleteOrders(double *totalOut) const
    {
        const vector<Order> orders = OrderManager::loadOrders(orderPath_);
        const vector<Bill> bills = loadBills();

        std::unordered_map<string, double> billTotals;
        for (const Bill &bill : bills)
        {
            billTotals[bill.orderId] += bill.totalPayable;
        }

        vector<OrderRow> rows;
        double total = 0;

        for (const Order &order : orders)
        {
            if (isCompleted(order.status))
            {
                continue;
            }

            OrderRow row;
            row.orderId = order.orderId;
            row.date = order.date;
            row.customerName = order.customerName;
            row.status = order.status;

            const auto found = billTotals.find(order.orderId);
            if (found != billTotals.end())
            {
                row.total = found->second;
            }
            else
            {
                row.total = quoteOrder(
                    order,
                    OrderManager::loadOrderDetails(order.orderId,
                                                   orderDetailPath_));
            }

            total += row.total;
            rows.push_back(row);
        }

        if (totalOut != nullptr)
        {
            *totalOut = BillCalculator::roundMoney(total);
        }
        return rows;
    }

    vector<OrderRow> ReportsService::completedOrders(double *totalOut) const
    {
        const vector<Order> orders = OrderManager::loadOrders(orderPath_);
        const vector<Bill> bills = loadBills();

        std::unordered_map<string, double> billTotals;
        for (const Bill &bill : bills)
        {
            billTotals[bill.orderId] += bill.totalPayable;
        }

        vector<OrderRow> rows;
        double total = 0;

        for (const Order &order : orders)
        {
            if (!isCompleted(order.status))
            {
                continue;
            }

            OrderRow row;
            row.orderId = order.orderId;
            row.date = order.date;
            row.customerName = order.customerName;
            row.status = order.status;

            const auto found = billTotals.find(order.orderId);
            if (found != billTotals.end())
            {
                row.total = found->second;
            }
            else
            {
                row.total = quoteOrder(
                    order,
                    OrderManager::loadOrderDetails(order.orderId,
                                                   orderDetailPath_));
            }

            total += row.total;
            rows.push_back(row);
        }

        if (totalOut != nullptr)
        {
            *totalOut = BillCalculator::roundMoney(total);
        }
        return rows;
    }

    vector<CashRow> ReportsService::cashSales(CashPeriod period,
                                              const string &fromDate,
                                              const string &toDate,
                                              const string &today,
                                              double *totalOut) const
    {
        const string day = today.empty() ? DateUtils::today() : today;
        if (!DateUtils::isValidDate(day))
        {
            throw std::invalid_argument("cash report needs a valid YYYY-MM-DD date");
        }

        string from;
        string to;
        bool groupByDay = true;

        switch (period)
        {
        case CashPeriod::Daily:
            from = day;
            to = day;
            break;

        case CashPeriod::Weekly:
            from = weekStart(day);
            to = addDays(from, 6);
            break;

        case CashPeriod::Monthly:
        {
            int year = 0;
            int month = 0;
            int ignored = 0;
            parseDate(day, year, month, ignored);
            from = formatDate(year, month, 1);
            to = formatDate(year, month, daysInMonth(year, month));
            break;
        }

        case CashPeriod::Yearly:
        {
            int year = 0;
            int month = 0;
            int dayOfMonth = 0;
            parseDate(day, year, month, dayOfMonth);
            from = formatDate(year, 1, 1);
            to = formatDate(year, 12, 31);
            groupByDay = false;
            break;
        }

        case CashPeriod::Custom:
            if (!DateUtils::isValidDate(fromDate) ||
                !DateUtils::isValidDate(toDate))
            {
                throw std::invalid_argument(
                    "custom cash report needs two valid YYYY-MM-DD dates");
            }
            if (fromDate > toDate)
            {
                throw std::invalid_argument(
                    "custom cash report start date is after the end date");
            }
            from = fromDate;
            to = toDate;
            break;
        }

        const vector<Order> orders = OrderManager::loadOrders(orderPath_);
        const vector<Bill> bills = loadBills();

        std::unordered_map<string, double> billTotals;
        for (const Bill &bill : bills)
        {
            billTotals[bill.orderId] += bill.totalPayable;
        }

        std::map<string, CashRow> grouped;

        for (const Order &order : orders)
        {
            if (order.status != "paid" || order.paymentMode != "cash")
            {
                continue;
            }
            if (!DateUtils::isValidDate(order.date))
            {
                continue;
            }
            if (order.date < from || order.date > to)
            {
                continue;
            }

            const auto found = billTotals.find(order.orderId);
            const double amount = found != billTotals.end()
                                      ? found->second
                                      : order.paymentAmount;

            const string key =
                groupByDay ? order.date : order.date.substr(0, 7);
            CashRow &row = grouped[key];
            row.period = key;
            row.amount = BillCalculator::roundMoney(row.amount + amount);
            row.orderCount += 1;
        }

        vector<CashRow> rows;
        rows.reserve(grouped.size());
        double total = 0;
        for (const auto &entry : grouped)
        {
            rows.push_back(entry.second);
            total += entry.second.amount;
        }

        if (totalOut != nullptr)
        {
            *totalOut = BillCalculator::roundMoney(total);
        }
        return rows;
    }

    vector<SalesRow> ReportsService::allSales(SalesTotals *totalsOut) const
    {
        const vector<Bill> bills = loadBills();

        vector<SalesRow> rows;
        rows.reserve(bills.size());

        SalesTotals totals;
        for (const Bill &bill : bills)
        {
            SalesRow row;
            row.billId = bill.billId;
            row.date = bill.billDate;
            row.customerName = bill.customerName;
            row.paymentStatus = bill.paymentStatus;
            row.totalNoTax = bill.totalNoTax;
            row.totalTax = bill.totalTax;
            row.productDiscount = bill.totalDiscount;
            row.seasonalDiscount = bill.seasonalDiscount;
            row.specialDiscount = specialDiscountAmount(bill);
            row.totalPayable = bill.totalPayable;

            totals.totalPayable += row.totalPayable;
            totals.productDiscount += row.productDiscount;
            totals.seasonalDiscount += row.seasonalDiscount;
            totals.specialDiscount += row.specialDiscount;

            rows.push_back(row);
        }

        if (totalsOut != nullptr)
        {
            totals.totalPayable = BillCalculator::roundMoney(totals.totalPayable);
            totals.productDiscount =
                BillCalculator::roundMoney(totals.productDiscount);
            totals.seasonalDiscount =
                BillCalculator::roundMoney(totals.seasonalDiscount);
            totals.specialDiscount =
                BillCalculator::roundMoney(totals.specialDiscount);
            *totalsOut = totals;
        }
        return rows;
    }

    vector<ItemRow> ReportsService::topSoldItems(std::size_t limit) const
    {
        const vector<Bill> bills = loadBills();

        std::unordered_map<string, double> sold;
        for (const Bill &bill : bills)
        {
            if (!isCompleted(bill.paymentStatus))
            {
                continue;
            }
            for (const BillLine &line : bill.lines)
            {
                sold[line.productAcronym] += line.quantity;
            }
        }

        std::unordered_map<string, const Product *> productsByAcronym;
        for (const Product &product : catalog_.all())
        {
            productsByAcronym[product.acronym()] = &product;
        }

        vector<ItemRow> rows;
        rows.reserve(sold.size());
        for (const auto &entry : sold)
        {
            ItemRow row;
            row.acronym = entry.first;
            row.quantity = BillCalculator::roundMoney(entry.second);

            const auto found = productsByAcronym.find(entry.first);
            if (found != productsByAcronym.end())
            {
                row.name = found->second->name();
                row.remainingStock = found->second->availableQuantity();
            }
            rows.push_back(row);
        }

        std::sort(rows.begin(), rows.end(),
                  [](const ItemRow &left, const ItemRow &right) {
                      if (left.quantity != right.quantity)
                          return left.quantity > right.quantity;
                      return left.acronym < right.acronym;
                  });

        if (rows.size() > limit)
        {
            rows.resize(limit);
        }
        return rows;
    }

    vector<ItemRow> ReportsService::topStockItems(std::size_t limit) const
    {
        vector<ItemRow> rows;
        rows.reserve(catalog_.all().size());

        for (const Product &product : catalog_.all())
        {
            ItemRow row;
            row.acronym = product.acronym();
            row.name = product.name();
            row.quantity = 0;
            row.remainingStock = product.availableQuantity();
            rows.push_back(row);
        }

        std::sort(rows.begin(), rows.end(),
                  [](const ItemRow &left, const ItemRow &right) {
                      if (left.remainingStock != right.remainingStock)
                          return left.remainingStock > right.remainingStock;
                      return left.acronym < right.acronym;
                  });

        if (rows.size() > limit)
        {
            rows.resize(limit);
        }
        return rows;
    }
} // namespace reports
