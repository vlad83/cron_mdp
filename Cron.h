#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <vector>

#include "AtValue.h"
#include "Monitor.h"
#include "Queue.h"
#include "json.h"

namespace cron {

using Clock = std::chrono::system_clock;

class Job
{
protected:
    /* canonical filename path - the job comes from,
     * will be used to remove job in case file is deleted/moved */
    std::string path_;
    AtValue atValue_;
    std::string service_;
    json payload_;

    friend
    Job parseJob(std::string path, const json &);
public:
    Job(std::string path, AtValue atValue, std::string service, json payload):
        path_{std::move(path)},
        atValue_{std::move(atValue)},
        service_{std::move(service)},
        payload_(std::move(payload))
    {}

    bool expired(Clock::time_point tp) const {return atValue_.expired(tp);}
    const std::string &service() const {return service_;}
    const json &payload() const {return payload_;}

    friend
    std::ostream &operator<< (std::ostream &, const Job &);
};

class Cron
{
    using JobSeq = std::vector<Job>;
    using JobSeqMap = std::map<std::string, JobSeq>;
    using EventSeq = Monitor::EventSeq;
    using EventSeqQueue = Queue<EventSeq>;

    std::string brokerAddr_;
    std::string basePath_;
    JobSeqMap jobSeqMap_;
    std::atomic<bool> stopMonitor_{false};
    std::atomic<bool> stopExec_{false};

    void update(const std::string &path);
    void update(const Monitor::EventSeq &);
    void dispatch(std::chrono::system_clock::time_point);
    void dispatch(const Job &);
    void monitor(EventSeqQueue &);
public:
    Cron(const std::string &brokerAddr, const std::string &basePath);
    void exec();
};

} /* cron */
