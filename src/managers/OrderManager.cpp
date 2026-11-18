#include "OrderManager.h"
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <cmath>

using namespace std;

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
    ifstream file("Order.txt");
    string line;
    while (getline(file, line)) {
        vector<string> t = split(line);
        if (t.size() < 2) continue;
        if (t[1] != date) continue;                       // date field only
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
    if (o.status != "Pending" && o.status != "Completed" && o.status != "Cancelled") return false;
    if (o.paymentMode != "Cash" && o.paymentMode != "Card" && o.paymentMode != "Online") return false;
    if (o.totalAmount < 0 || o.paymentAmount < 0) return false;
    if (details.empty()) return false;

    for (size_t i = 0; i < details.size(); i++) {
        const OrderDetail& d = details[i];
        if (d.orderId != o.orderId) return false;
        if (d.productId <= 0 || d.quantity <= 0 || d.unitPrice < 0) return false;
        if (fabs(d.subtotal - d.quantity * d.unitPrice) > 0.01) return false;
    }
    return true;
}

bool OrderManager::saveOrderTransaction(const Order& o, const vector<OrderDetail>& details) {
    if (!validateOrder(o, details)) return false;

    ofstream detailFile("OrderDetail.txt", ios::app);
    ofstream orderFile("Order.txt", ios::app);
    if (!detailFile.is_open() || !orderFile.is_open()) return false;

    for (size_t i = 0; i < details.size(); i++) {
        detailFile << details[i].orderId << " " << details[i].productId << " "
                   << to_string(details[i].quantity) << " " << to_string(details[i].unitPrice) << " "
                   << to_string(details[i].subtotal) << " #\n";
    }

    orderFile << o.orderId << " " << o.date << " " << o.time << " "
              << sanitize(o.customerName) << " " << sanitize(o.contact) << " "
              << sanitize(o.merchant) << " " << to_string(o.totalAmount) << " "
              << to_string(o.paymentAmount) << " " << o.status << " "
              << o.paymentMode << " #\n";

    return detailFile.good() && orderFile.good();
}

vector<Order> OrderManager::loadOrders() {
    vector<Order> orders;
    ifstream file("Order.txt");
    string line;
    while (getline(file, line)) {
        vector<string> t = split(line);
        if (t.size() != 11 || t[10] != "#") continue;
        Order o;
        o.orderId = t[0];
        o.date = t[1];
        o.time = t[2];
        o.customerName = desanitize(t[3]);
        o.contact = desanitize(t[4]);
        o.merchant = desanitize(t[5]);
        o.totalAmount = atof(t[6].c_str());
        o.paymentAmount = atof(t[7].c_str());
        o.status = t[8];
        o.paymentMode = t[9];
        orders.push_back(o);
    }
    return orders;
}

vector<OrderDetail> OrderManager::loadOrderDetails(const string& orderId) {
    vector<OrderDetail> details;
    ifstream file("OrderDetail.txt");
    string line;
    while (getline(file, line)) {
        vector<string> t = split(line);
        if (t.size() != 6 || t[5] != "#" || t[0] != orderId) continue;
        OrderDetail d;
        d.orderId = t[0];
        d.productId = atoi(t[1].c_str());
        d.quantity = atof(t[2].c_str());
        d.unitPrice = atof(t[3].c_str());
        d.subtotal = atof(t[4].c_str());
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