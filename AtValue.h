#pragma once

#include <algorithm>
#include <chrono>
#include <ostream>

#include "json.h"

namespace cron {

struct AtValue;

AtValue parseAtValue(const json &);

struct AtValue
{
    using Clock = std::chrono::system_clock;
    using Seq = std::vector<int>;

    friend
    AtValue parseAtValue(const json &);

    friend
    std::ostream &operator<<(std::ostream &, const AtValue &);

    bool expired(Clock::time_point) const;
private:
    /* invariant: all sequences are sorted */
    Seq seconds_; /* 0..59 */
    Seq minutes_; /* 0..59 */
    Seq hours_; /* 0..23 */
    Seq weekdays_; /* 0..6 */
    Seq monthdays_; /* 0-31 */
    Seq months_; /* 0..11 */
protected:
    AtValue(
        Seq seconds,
        Seq minutes,
        Seq hours,
        Seq weekdays,
        Seq monthdays,
        Seq months):
        seconds_{std::move(seconds)},
        minutes_{std::move(minutes)},
        hours_{std::move(hours)},
        weekdays_{std::move(weekdays)},
        monthdays_{std::move(monthdays)},
        months_{std::move(months)}
    {}
};


inline
AtValue::Seq sort(AtValue::Seq seq)
{
    std::sort(std::begin(seq), std::end(seq));
    return seq;
}

inline
bool includes(const AtValue::Seq &seq, int value)
{
    if(seq.empty()) return false;
    return std::binary_search(std::begin(seq), std::end(seq), value);
}

} /* cron */
