#include <ctime>
#include <iomanip>

#include "AtValue.h"
#include "Ensure.h"
#include "Trace.h"

namespace {

constexpr auto AT = "at";
constexpr auto SECONDS = "second";
constexpr auto MINUTES = "minute";
constexpr auto HOURS = "hour";
constexpr auto WEEK_DAYS = "week_day";
constexpr auto MONTH_DAYS = "month_day";
constexpr auto MONTHS = "month";

std::vector<int> validate(std::vector<int> seq, const int min, const int max)
{
    for(auto i = std::begin(seq); i != std::end(seq); ++i)
    {
        ENSURE(min <= *i && max >= *i, RuntimeError);
    }
    return seq;
}

} /* namespace */

namespace cron {

AtValue parseAtValue(const json &input)
{
    ENSURE(input.count(AT), RuntimeError);
    ENSURE(input[AT].is_object(), RuntimeError);

    AtValue::Seq seconds, minutes, hours, weekdays, monthdays, months;

    if(input[AT].count(SECONDS))
    {
        ENSURE(input[AT][SECONDS].is_array(), RuntimeError);

        seconds = validate(input[AT][SECONDS].get<AtValue::Seq>(), 0, 59);
    }

    if(input[AT].count(MINUTES))
    {
        ENSURE(input[AT][MINUTES].is_array(), RuntimeError);

        minutes = validate(input[AT][MINUTES].get<AtValue::Seq>(), 0, 59);
    }

    if(input[AT].count(HOURS))
    {
        ENSURE(input[AT][HOURS].is_array(), RuntimeError);

        hours = validate(input[AT][HOURS].get<AtValue::Seq>(), 0, 23);
    }

    if(input[AT].count(WEEK_DAYS))
    {
        ENSURE(input[AT][WEEK_DAYS].is_array(), RuntimeError);

         weekdays = validate(input[AT][WEEK_DAYS].get<AtValue::Seq>(), 1, 7);
    }

    if(input[AT].count(MONTH_DAYS))
    {
        ENSURE(input[AT][MONTH_DAYS].is_array(), RuntimeError);

         monthdays = validate(input[AT][MONTH_DAYS].get<AtValue::Seq>(), 1, 31);
    }

    if(input[AT].count(MONTHS))
    {
        ENSURE(input[AT][MONTHS].is_array(), RuntimeError);

         months = validate(input[AT][MONTHS].get<AtValue::Seq>(), 1, 12);
    }

    return
        AtValue
        {
            sort(std::move(seconds)),
            sort(std::move(minutes)),
            sort(std::move(hours)),
            sort(std::move(weekdays)),
            sort(std::move(monthdays)),
            sort(std::move(months))
        };
}

std::ostream &operator<<(std::ostream &os, const AtValue &atValue)
{
    const auto flags = os.flags();

    const auto dumpSeq = [&os](const AtValue::Seq &seq)
    {
        for(auto begin = std::begin(seq); std::end(seq) != begin; ++begin)
        {
            os << *begin;
            if(std::next(begin) != std::end(seq)) os << ',';
        }
    };

    os << "s";
    dumpSeq(atValue.seconds_);
    os << "m";
    dumpSeq(atValue.minutes_);
    os << "h";
    dumpSeq(atValue.hours_);
    os << "wd";
    dumpSeq(atValue.weekdays_);
    os << "md";
    dumpSeq(atValue.monthdays_);
    os << "M";
    dumpSeq(atValue.months_);

    os.flags(flags);
    return os;
}

bool AtValue::expired(Clock::time_point tp) const
{
    const auto time = Clock::to_time_t(tp);
    auto *tm = ::std::localtime(&time);

    ENSURE(tm, CRuntimeError);

    if(!months_.empty() && !includes(months_, tm->tm_mon)) return false;
    if(!monthdays_.empty() && !includes(monthdays_, tm->tm_mday)) return false;
    if(!weekdays_.empty() && !includes(weekdays_, tm->tm_wday)) return false;
    if(!hours_.empty() && !includes(hours_, tm->tm_hour)) return false;
    if(!minutes_.empty() && !includes(minutes_, tm->tm_min)) return false;
    if(!seconds_.empty() && !includes(seconds_, tm->tm_sec)) return false;

    return true;
}

} /* cron */
