#ifndef ORDER_POLICY_H
#define ORDER_POLICY_H

#include <string>
#include "../models/Order.h"

// Timestamp of the bill belonging to an order, if one exists. OrderPolicy only
// needs the date, so it does not depend on Person C's Bill model; the caller
// fills this from Bill.billDate / Bill.billTime (hasBill = false when the
// order has no bill yet).
struct BillStamp {
    bool hasBill = false;
    std::string date;   // YYYY-MM-DD
    std::string time;   // HH:MM:SS (informational; the rule works on dates)
};

// Qt-free rules for editing orders (requirement 3).
//
//   archived              -> never editable
//   on-hold / in-progress -> always editable
//   paid                  -> editable directly while the bill (or, with no
//                            bill, the order) is at most 7 days old;
//                            older than that it needs manager approval.
//
// "Days old" is whole calendar days between the stamp date and `today`, so
// day 7 is still direct and day 8 needs approval.
class OrderPolicy {
public:
    static constexpr int kPaidEditWindowDays = 7;

    static bool isUnpaid(const std::string &status);   // in-progress/on-hold

    // Allowed status changes. Same-status is always allowed; archived is final.
    //   in-progress -> on-hold | paid | archived
    //   on-hold     -> in-progress | paid | archived
    //   paid        -> archived
    static bool canTransition(const std::string &from, const std::string &to);

    // True when a paid order's stamp is within the 7-day window.
    // An invalid date (stamp or today) counts as outside the window.
    static bool isWithinPaidEditWindow(const Order &order,
                                       const BillStamp &bill,
                                       const std::string &today);

    // Paid and outside the 7-day window.
    static bool requiresManagerApproval(const Order &order,
                                        const BillStamp &bill,
                                        const std::string &today);

    // `managerApproved` is the per-order, per-session approval from the UI.
    static bool canEdit(const Order &order,
                        const BillStamp &bill,
                        const std::string &today,
                        bool managerApproved = false);
};

#endif