/**
 * Algorithms to determine whether a date is in daylight saving time.
 * Subclasses determine behavior such as never, EU1996, US2007.
 * David Levine <levined@ieee.org> 20200405
 */

#include "dst.h"

const DSTnever DST::dstNever;
const DSTeu1996 DST::dstEU1996;
const DSTus2007 DST::dstUS2007;
const DST *DST::dst = &DST::dstNever;

void DST::setDSTMode(unsigned int mode)
{
    switch (mode)
    {
        case DST_EU1996: dst = &dstEU1996; break;
        case DST_US2007: dst = &dstUS2007; break;
        default:         dst = &dstNever;
    }
}

bool
DST::isDST(tmElements_t &tm, int hoursOffset, bool wasDST)
{
    return dst->isDST_i(tm, hoursOffset, wasDST);
}

DST::~DST()
{
}

bool
DSTnever::isDST_i(tmElements_t &, int, bool) const
{
    return false;
}

bool
DSTeu1996::isDST_i(tmElements_t &tm, int hoursOffset, bool wasDST) const
{
    // EU changes to/from DST based on UTC.  tm is local so convert it.
    const time_t utc = makeTime(tm) - (hoursOffset + (wasDST ? 1 : 0)) * 3600;
    const int utc_hour = hour(utc), utc_day = day(utc),
        utc_wday = weekday(utc), utc_month = month(utc);

    if (utc_month < 3 || utc_month > 10) {
        return false;
    } else if (utc_month > 3 && utc_month < 10) {
        return true;
    } else if (utc_month == 3) {
        // mday1 is the mday of the Sunday of this week
        const int mday1 = utc_day - utc_wday + 1;
        if (utc_wday == 1 && mday1 > 24) {
            // last Sunday of month 3, DST only after 0100
            return utc_hour >= 1;
        } else {
            return mday1 > 24;
        }
    } else {
        // month == 10
        // mday1 is the mday of the Sunday of this week
        const int mday1 = utc_day - utc_wday + 1;
        if (utc_wday == 1 && mday1 > 24) {
            // last Sunday of month 10, DST only before 0100
            // Between 0000 and 0059, can be either depending on whether
            // clock was already set back.
            return utc_hour == 0 ? wasDST : false;
        }
        return mday1 < 25;
    }
}

bool
DSTus2007::isDST_i(tmElements_t &tm, int, bool wasDST) const
{
    if (tm.Month < 3 || tm.Month > 11) {
        return false;
    } else if (tm.Month > 3 && tm.Month < 11) {
        return true;
    } else if (tm.Month == 3) {
        // mday1 is the mday of the Sunday of this week
        const int mday1 = tm.Day - tm.Wday + 1;
        if (tm.Wday == 1 && mday1 > 7 && mday1 < 15) {
            // second Sunday of March, DST only after 0200
            return tm.Hour >= 2;
        } else {
            return mday1 > 7;
        }
    } else {
        // month == 11
        // mday1 is the mday of the Sunday of this week
        const int mday1 = tm.Day - tm.Wday + 1;
        if (tm.Wday == 1 && mday1 < 8) {
            // first Sunday of November, can be DST only before 0200
            // Between 0100 and 0159, can be either depending on whether
            // clock was already set back.
            switch (tm.Hour)
            {
                case 0: return true;
                case 1: return wasDST;
                default: return false;
            }
        }
        return mday1 < 1;
    }
}
