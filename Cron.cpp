//#include <algorithm>
//#include <chrono>
#include <fstream>
#include <future>
//#include <iterator>
//#include <limits>

//#include <unistd.h>

#include "Cron.h"
#include "Ensure.h"
#include "Trace.h"
#include "fs.h"
#include "mdp/Client.h"
#include "mdp/MDP.h"

namespace {
constexpr auto SERVICE = "service";
constexpr auto PAYLOAD = "payload";

struct StopGuard
{
    std::atomic<bool> &stop_;

    StopGuard(std::atomic<bool> &stop): stop_{stop} {}
    ~StopGuard() {stop_ =true;}
};

}

namespace cron {

Job parseJob(std::string path, const json &input)
{
    const auto atValue = parseAtValue(input);

    ENSURE(input.count(SERVICE), RuntimeError);
    ENSURE(input[SERVICE].is_string(), RuntimeError);

    const auto service = input[SERVICE].get<std::string>();

    ENSURE(input.count(PAYLOAD), RuntimeError);
    ENSURE(input[PAYLOAD].is_array(), RuntimeError);

    const json payload = input[PAYLOAD].get<json>();

    return
    {
        std::move(path),
        std::move(atValue),
        std::move(service),
        std::move(payload)
    };
}

std::ostream &operator<<(std::ostream &os, const Job &job)
{
    os
        << job.atValue_
        << ' ' << job.path_
        << ' ' << job.service_
        << ' ' << job.payload_.dump();
    return os;
}

Cron::Cron(const std::string &brokerAddr, const std::string &basePath):
    brokerAddr_{std::move(brokerAddr)},
    basePath_{resolvePath(basePath)}
{
    ENSURE(isDirectory(basePath_), RuntimeError);

    const auto list = listDirectory(basePath_);

    for(const auto &name : list)
    {
        const auto path = basePath_ + '/' + name;

        if(!isRegularFile(path)) continue;
        if(!isExtention(path, ".json")) continue;

        update(path);
    }
}

void Cron::update(const std::string &path)
{
    TRACE(TraceLevel::Debug, path);

    /* erase any existing */
    jobSeqMap_.erase(path);
    /* if file is not accessible ignore it */
    if(!access(path, AccessMode::Exist | AccessMode::Read)) return;

    json input;
    std::ifstream{path} >> input;

    ENSURE(input.is_array(), RuntimeError);

    JobSeq seq;

    for(const auto &i : input)
    {
        seq.push_back(parseJob(path, i));
        LOG(TraceLevel::Debug, seq.back());
    }

    if(!seq.empty()) jobSeqMap_[path] = seq;
}

void Cron::update(const Monitor::EventSeq &eventSeq)
{
    for(const auto &event : eventSeq)
    {
        using EventType = Monitor::EventType;

        ASSERT(!event.isEvent(EventType::DeleteSelf));
        ASSERT(!event.isEvent(EventType::MoveSelf));

        const auto path = event.path();

        //TRACE(TraceLevel::Debug, event);

        // check if path still exists
        if(!access(path, AccessMode::Exist | AccessMode::Read)) continue;
        if(!isRegularFile(path)) continue;
        if(!isExtention(path, ".json")) continue;

        update(path);
    }
}

void Cron::dispatch(std::chrono::system_clock::time_point now)
{
    for(const auto &jobSeq : jobSeqMap_)
    {
        for(const auto &job : jobSeq.second)
        {
            if(job.expired(now)) dispatch(job);
        }
    }
}

void Cron::dispatch(const Job &job)
{
    TRACE(TraceLevel::Debug, "job ", job);

    Client client;

    std::vector<std::string> requestPayload;

    for(const auto &i : job.payload()) requestPayload.emplace_back(i.dump());

    const auto replyPayload =
        client.exec(brokerAddr_, job.service(), requestPayload);

    ENSURE(2 == int(replyPayload.size()), RuntimeError);
    ENSURE(MDP::Broker::Signature::statusSucess == replyPayload[0], RuntimeError);
}

void Cron::exec()
{
    while(!stopExec_)
    {
        TRACE(TraceLevel::Info, "stopMonitor_ ", stopMonitor_);
        stopMonitor_ = false;

        try
        {
            EventSeqQueue queue;

            auto r =
                std::async(
                    std::launch::async,
                    [this, &queue]()
                    {
                        monitor(queue);
                    });

            /* in case current thread throws make sure async task
             * is terminated otherwise deadlock will occur */
            StopGuard stopGuard{stopMonitor_};

            /* delay dispatching to timeout failed jobs (on restart) */
            std::this_thread::sleep_for(std::chrono::seconds{1});

            while(!stopExec_)
            {
                EventSeq eventSeq;

                if(queue.pop(eventSeq, std::chrono::milliseconds{50}))
                {
                    update(eventSeq);
                }

                dispatch(Clock::now());
            }

            /* if async thread throws exception it will be propagated on get() */
            r.get();
        }
        catch(const std::exception &except)
        {
            TRACE(TraceLevel::Error, except.what());
        }
        catch(...)
        {
            TRACE(TraceLevel::Error, "unsupported exception");
            stopExec_ = true;
            stopMonitor_ = true;
        }
    }
}

void Cron::monitor(EventSeqQueue &queue)
{
    while(!stopMonitor_)
    {
        try
        {
            using EventType = Monitor::EventType;

            Monitor monitor;

            monitor.add(
                basePath_,
                /* modified inside watched directory */
                EventType::CloseWrite
                /* create inside watched directory (link to file) */
                | EventType::Create
                /* moved from watched directory */
                | EventType::MovedFrom
                /* moved into watched directory (potentially overwriting) */
                | EventType::MovedTo
                /* watched directory moved (fatal) */
                | EventType::MoveSelf
                /* file deleted inside watched directory */
                | EventType::Delete
                /* watched directory deleted (fatal) */
                | EventType::DeleteSelf);

            while(!stopMonitor_)
            {
                const auto eventSeq = monitor.poll(Monitor::mSecs{500});

                if(!eventSeq.empty())
                {
                    queue.push(std::move(eventSeq));
                }
            }
        }
        catch(const std::exception &except)
        {
            TRACE(TraceLevel::Error, except.what());
        }
        catch(...)
        {
            TRACE(TraceLevel::Error, "unsupported exception");
            stopExec_ = true;
        }
    }
}

} /* cron */
