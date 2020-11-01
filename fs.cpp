#include <algorithm>
#include <cstring>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fs.h"
#include "Ensure.h"

namespace {

struct ::stat statPath(const std::string &path)
{
    ENSURE(!path.empty(), RuntimeError);

    struct ::stat s;

    ENSURE(0 == ::stat(path.c_str(), &s), CRuntimeError);
    return s;
}

} /* namespace */

bool access(const std::string &path, AccessMode mode)
{
    const auto status = ::access(path.c_str(), int(mode));

    //if(0 != status) TRACE(TraceLevel::Debug, ::strerror(errno));
    return 0 == status;
}

bool isDirectory(const std::string &path)
{
    return S_ISDIR(statPath(path).st_mode);
}

bool isRegularFile(const std::string &path)
{
    return S_ISREG(statPath(path).st_mode);
}

bool isLink(const std::string &path)
{
    return S_ISLNK(statPath(path).st_mode);
}

PathSeq listDirectory(const std::string &path)
{
    if(path.empty()) return {};

    auto *dir = ::opendir(path.c_str());

    ENSURE(dir, CRuntimeError);

    //TRACE(TraceLevel::Debug, path);

    PathSeq seq;

    for(;;)
    {
        errno = 0;

        auto entry = ::readdir(dir);

        if(entry)
        {
            seq.emplace_back(entry->d_name);
            continue;
        }

        ENSURE(0 == errno, CRuntimeError);
        break;
    }

    ENSURE(0 == ::closedir(dir), CRuntimeError);

    return seq;
}

std::string resolvePath(const std::string &path)
{
    ENSURE(!path.empty(), RuntimeError);

    char buf[PATH_MAX] = {'\0'};

    ENSURE(::realpath(path.c_str(), buf), CRuntimeError);

    const std::string rpath{buf};

    //TRACE(TraceLevel::Debug, "path ", path, " resolved to ", rpath);
    return rpath;
}

bool isExtention(const std::string &path, const std::string &ext)
{
    const auto i =
        std::find_end(
            std::begin(path), std::end(path),
            std::begin(ext), std::end(ext));

    if(std::end(path) == i) return false;
    if(std::distance(i, std::end(path)) != int(ext.size())) return false;
    return true;
}
