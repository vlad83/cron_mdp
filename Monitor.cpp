#include <climits>
#include <iomanip>

#include <errno.h>
#include <poll.h>
#include <unistd.h>

#include <sys/inotify.h>

#include "Ensure.h"
#include "Monitor.h"
#include "Trace.h"

namespace {

void validateSysCallResult(int r)
{
    const auto valid = -1 != r || (-1 == r && EINTR == errno);
    ENSURE(valid, CRuntimeError);
}

} /* namespace */

std::ostream &operator<<(std::ostream &os, const Monitor::EventMask mask)
{
    using EventType = Monitor::EventType;

    if(int(EventType::Access) & mask.value) os << "IN_ACCESS ";
    if(int(EventType::Attrib) & mask.value) os << "IN_ATTRIB ";
    //if(int(EventType::Close) & mask.value) os << "IN_CLOSE ";
    if(int(EventType::CloseWrite) & mask.value) os << "IN_CLOSE_WRITE ";
    if(int(EventType::CloseNoWrite) & mask.value) os << "IN_CLOSE_NOWRITE ";
    if(int(EventType::Create) & mask.value) os << "IN_CREATE ";
    if(int(EventType::Delete) & mask.value) os << "IN_DELETE ";
    if(int(EventType::DeleteSelf) & mask.value) os << "IN_DELETE_SELF ";
    if(int(EventType::Modify) & mask.value) os << "IN_MODIFY ";
    //if(int(EventType::Move) & mask.value) os << "IN_MOVE ";
    if(int(EventType::MoveSelf) & mask.value) os << "IN_MOVE_SELF ";
    if(int(EventType::MovedFrom) & mask.value) os << "IN_MOVED_FROM ";
    if(int(EventType::MovedTo) & mask.value) os << "IN_MOVED_TO ";
    if(int(EventType::Open) & mask.value) os << "IN_OPEN ";
    return os;
}

std::ostream &operator<<(std::ostream &os, const Monitor::Event &event)
{
    const auto flags = os.flags();
    os
        << '#' << event.fd()
        << ' ' << event.path()
        << ' ' << event.name()
        << ' ' << std::hex << std::setw(4) << std::setfill('0') << event.mask().value
        << ' ' << event.mask();
    os.flags(flags);
    return os;
}

Monitor::Monitor()
{
    /* create inotify event queue.
     * Make all op none blocking.
     * Close this file descriptor in child process if parent forks */
    fd_ = ::inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

    ENSURE(-1 != fd_, CRuntimeError);
}

Monitor::~Monitor()
{
    if(-1 != fd_)
    {
        ::close(fd_);
        fd_ = -1;
    }
}

void Monitor::add(std::string path, EventType event)
{
    ENSURE(-1 != fd_, RuntimeError);
    ENSURE(!path.empty(), RuntimeError);
    /* make sure caller has read permission to access the path */
    ENSURE(-1 != ::access(path.c_str(), R_OK), CRuntimeError);

    const auto wfd = ::inotify_add_watch(fd_, path.c_str(), int(event));

    ENSURE(-1 != wfd, CRuntimeError);

    map_.emplace(wfd, INode{wfd, std::move(path)});
}

auto Monitor::poll(mSecs timeout) -> EventSeq
{
    ENSURE(-1 != fd_, RuntimeError);

    using namespace std::chrono;

    EventSeq eventSeq;
    const auto timestamp = steady_clock::now();
    auto elapsed{steady_clock::duration{0}};

    while(timeout >= elapsed && eventSeq.empty())
    {
        {
            struct pollfd events =
            {
                fd_,
                short(POLLIN) /* events */,
                short(0) /* revents */
            };
            auto r = ::poll(&events, 1, (timeout - duration_cast<mSecs>(elapsed)).count());

            // ignore poll interrupted by received signal
            validateSysCallResult(r);
            elapsed = steady_clock::now() - timestamp;
            /* fd is not ready for reading */
            if(0 == r) continue;
            if(0 == (events.revents & POLLIN)) continue;
        }


        for(;;)
        {
            //TRACE(TraceLevel::Debug, "event");

            char buf[sizeof(::inotify_event) + NAME_MAX + 1];
            auto r = ::read(fd_, buf, sizeof(buf));

            /* fd_ is non-blocking (no data to read) */
            if(-1 == r && EAGAIN == errno) break;

            validateSysCallResult(r);

            ENSURE(0 != r, RuntimeError);
            /* check if at least 1 inotify_event struct was read */
            ENSURE(int(sizeof(::inotify_event)) <= r, RuntimeError);

            const auto *event = reinterpret_cast<const ::inotify_event *>(buf);

            if(map_.count(event->wd))
            {
                eventSeq.emplace_back(
                    event->wd,
                    map_[event->wd].path(),
                    event->mask,
                    std::string(event->name));
            }
        }
    }

    return eventSeq;
}
