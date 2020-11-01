#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>

#include <sys/inotify.h>

struct Monitor
{
    enum class EventType : int
    {
        /* see inotify for detailed event desciption */
        Access = IN_ACCESS,
        Attrib  = IN_ATTRIB,
        Close = IN_CLOSE,
        CloseWrite = IN_CLOSE_WRITE,
        CloseNoWrite = IN_CLOSE_NOWRITE,
        Create = IN_CREATE,
        Delete = IN_DELETE,
        DeleteSelf = IN_DELETE_SELF,
        Modify = IN_MODIFY,
        Move = IN_MOVE,
        MoveSelf = IN_MOVE_SELF,
        MovedFrom = IN_MOVED_FROM,
        MovedTo = IN_MOVED_TO,
        Open = IN_OPEN,
        All = IN_ALL_EVENTS
    };

    struct EventMask
    {
        uint32_t value;

        EventMask(uint32_t v = 0): value{v}
        {}

        friend
        std::ostream &operator<<(std::ostream &, EventMask);
    };

    friend
    EventType operator|(EventType x, EventType y)
    {
        return EventType(int(x) | int(y));
    }

    using mSecs  = std::chrono::milliseconds;

    class INode
    {
        int fd_ = {-1};
        std::string path_;
    public:
        INode()
        {}

        INode(int fd, std::string path):
            fd_{fd}, path_{std::move(path)}
        {}

        int fd() const {return fd_;}
        const std::string &path() const {return path_;}
    };

    using Map = std::map<int, INode>;

    class Event
    {
        int fd_;
        std::string basePath_;
        EventMask mask_;
        std::string name_;
    public:
        Event(int fd, std::string basePath, uint32_t mask, std::string name):
            fd_{fd}, basePath_{std::move(basePath)}, mask_{mask}, name_{std::move(name)}
        {}

        int fd() const {return fd_;}
        const std::string &basePath() const {return basePath_;}
        const std::string &name() const {return name_;}
        std::string path() const {return basePath_ + '/' + name_;}
        EventMask mask() const {return mask_;}
        bool isEvent(EventType event) const {return 0 != (int(event) & mask_.value);}

        friend
        std::ostream &operator<<(std::ostream &, const Event &);
    };

    using EventSeq = std::vector<Event>;
private:
    int fd_ = {-1};
    Map map_;
public:
    Monitor();
    ~Monitor();
    void add(std::string path, EventType);
    EventSeq poll(mSecs timeout);
};
