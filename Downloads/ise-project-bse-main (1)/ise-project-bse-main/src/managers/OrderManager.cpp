#include "OrderManager.h"
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace std;

// --
static const string ORDER_FILE_PATH = "Order.txt";
static const string DETAIL_FILE_PATH = "OrderDetail.txt";
static string sanitize(string s) {
    if (s.empty()) return "-";
    for (size_t i = 0; i < s.size(); i++)
        if (s[i] == ' ' || s[i] == '\t' || s[i] == '#') s[i] = '_';
    return s;
}

static string desanitize(string s) {
    if (s == "-") return "";
    for (size_t i = 0; i < s.size(); i++)
        if (s[i] == '_') s[i] = ' ';
    return s;
}

static vector<string> split(const string& line) {
    vector<string> tokens;
    size_t start = 0;
    while (start < line.size()) {
        size_t pos = line.find(' ', start);
        if (pos == string::npos) pos = line.size();
        if (pos > start) tokens.push_back(line.substr(start, pos - start));
        start = pos + 1;
    }
    return tokens;
}

static bool isDigits(const string& s) {
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); i++)
        if (s[i] < '0' || s[i] > '9') return false;
    return true;
}

static bool validDate(const string& d) {
    return d.size() == 10 && d[4] == '-' && d[7] == '-' &&
           isDigits(d.substr(0, 4)) && isDigits(d.substr(5, 2)) && isDigits(d.substr(8, 2));
}

static string mmdd(const string& date) {
    return date.substr(5, 2) + date.substr(8, 2);
}

static string fmt(double v) {
    ostringstream o;
    o << fixed << setprecision(2) << v;
    return o.str();
}

string OrderManager::getCurrentDate() {
    time_t now = time(0);
    char buf[16];
    strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&now));
    return buf;
}

string OrderManager::getCurrentTime() {
    time_t now = time(0);
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now));
    return buf;
}

string OrderManager::generateOrderId(const string& dateIn) {
    string date = dateIn.empty() ? getCurrentDate() : dateIn;
    if (!validDate(date)) return "";

    int maxSeq = 0;
    ifstream file(ORDER_FILE_PATH); // Updated Path
    string line;
    while (getline(file, line)) {
        vector<string> t = split(line);
        if (t.size() < 2) continue;
        if (t[1] != date) continue;
        if (t[0].size() != 8 || !isDigits(t[0])) continue;
        int seq = atoi(t[0].substr(0, 4).c_str());
        if (seq > maxSeq) maxSeq = seq;
    }

    if (maxSeq >= 9999) return "";
    string seq = to_string(maxSeq + 1);
    while (seq.size() < 4) seq = "0" + seq;
    return seq + mmdd(date);
}

bool OrderManager::validateOrder(const Order& o, const vector<OrderDetail>& details) {
    if (o.orderId.size() != 8 || !isDigits(o.orderId)) return false;
    if (!validDate(o.date) || o.orderId.substr(4) != mmdd(o.date)) return false;

    if (o.status != "in-progress" && o.status != "on-hold" &&
        o.status != "paid" && o.status != "archived") return false;
    if (o.paymentMode != "cash" && o.paymentMode != "card" &&
        o.paymentMode != "wallet") return false;

    if (o.paymentAmount < 0) return false;
    if (details.empty()) return false;

    for (size_t i = 0; i < details.size(); i++) {
        const OrderDetail& d = details[i];
        if (d.orderId != o.orderId) return false;
        if (d.productId <= 0 || d.quantity <= 0) return false;
        if (d.barcode.empty()) return false;
    }
    return true;
}

bool OrderManager::saveOrderTransaction(const Order& o, const vector<OrderDetail>& details) {
    if (!validateOrder(o, details)) return false;

    ofstream detailFile(DETAIL_FILE_PATH, ios::app); // Updated Path
    ofstream orderFile(ORDER_FILE_PATH, ios::app);   // Updated Path
    if (!detailFile.is_open() || !orderFile.is_open()) return false;

    for (size_t i = 0; i < details.size(); i++) {
        detailFile << details[i].orderId << " "
                   << details[i].productId << " "
                   << details[i].barcode << " "
                   << details[i].quantity << " #\n";
    }

    orderFile << o.orderId << " "
              << o.date << " "
              << o.time << " "
              << sanitize(o.customerName) << " "
              << sanitize(o.contact) << " "
              << o.status << " "
              << fmt(o.paymentAmount) << " "
              << o.paymentMode << " "
              << sanitize(o.merchant) << " #\n";

    return detailFile.good() && orderFile.good();
}

vector<Order> OrderManager::loadOrders() {
    vector<Order> orders;
    ifstream file(ORDER_FILE_PATH); // Updated Path
    string line;
    while (getline(file, line)) {
        vector<string> t = split(line);
        if (t.size() != 10 || t[9] != "#") continue;
        Order o;
        o.orderId       = t[0];
        o.date          = t[1];
        o.time          = t[2];
        o.customerName  = desanitize(t[3]);
        o.contact       = desanitize(t[4]);
        o.status        = t[5];
        o.paymentAmount = atof(t[6].c_str());
        o.paymentMode   = t[7];
        o.merchant      = desanitize(t[8]);
        orders.push_back(o);
    }
    return orders;
}

vector<OrderDetail> OrderManager::loadOrderDetails(const string& orderId) {
    vector<OrderDetail> details;
    ifstream file(DETAIL_FILE_PATH); // Updated Path
    string line;
    while (getline(file, line)) {
        vector<string> t = split(line);
        if (t.size() != 5 || t[4] != "#" || t[0] != orderId) continue;
        OrderDetail d;
        d.orderId   = t[0];
        d.productId = atoi(t[1].c_str());
        d.barcode   = t[2];
        d.quantity  = atoi(t[3].c_str());
        details.push_back(d);
    }
    return details;
}

bool OrderManager::findOrder(const string& orderId, Order& result) {
    vector<Order> orders = loadOrders();
    for (size_t i = 0; i < orders.size(); i++) {
        if (orders[i].orderId == orderId) {
            result = orders[i];
            return true;
        }
    }
    return false;
}