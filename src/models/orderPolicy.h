#ifndef ORDERPOLICY_H
#define ORDERPOLICY_H

#include <string>
#include <sstream>
#include <ctime>

class OrderPolicy
{
private:
    static std::time_t parseDate(const std::string& dateStr) {
        std::tm tm = {};
        std::istringstream ss(dateStr);
        char dash1, dash2;
        ss >> tm.tm_year >> dash1 >> tm.tm_mon >> dash2 >> tm.tm_mday;
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        return std::mktime(&tm);
    }

public:
    static int daysBetween(const std::string& startDateStr, const std::string& endDateStr) {
        std::time_t t1 = parseDate(startDateStr);
        std::time_t t2 = parseDate(endDateStr);
        double diffSeconds = std::difftime(t2, t1);
        return static_cast<int>(diffSeconds / (60 * 60 * 24));
    }

    static bool requiresManagerApproval(const std::string& status, const std::string& orderDate, 
                                        const std::string& billDate, const std::string& currentDate) {
        if (status != "paid" && status != "Completed") return false;
        
        std::string refDate = billDate.empty() ? orderDate : billDate;
        if (refDate.length() >= 10 && currentDate.length() >= 10) {
            std::string refDay = refDate.substr(0, 10);
            std::string currDay = currentDate.substr(0, 10);
            int days = daysBetween(refDay, currDay);
            return days > 7;
        }
        return false;
    }

    static bool canEdit(const std::string& status, const std::string& orderDate, 
                        const std::string& billDate, const std::string& currentDate, 
                        bool managerApproved = false) {
        if (status == "archived") return false;
        if (status == "on-hold" || status == "in-progress" || status == "Pending") return true;

        if (status == "paid" || status == "Completed") {
            if (requiresManagerApproval(status, orderDate, billDate, currentDate)) {
                return managerApproved;
            }
            return true; 
        }
        return true;
    }
};

#endif ORDERPOLICY_H
