#include <string>
#include <vector>

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

using PathSeq = std::vector<std::string>;

enum class AccessMode : int
{
    Exist = F_OK,
    Read = R_OK,
    Write = W_OK,
    Execute = X_OK
};

inline
AccessMode operator|(AccessMode x, AccessMode y)
{
    return AccessMode(int(x) | int(y));
}

bool access(const std::string &path, AccessMode);
bool isDirectory(const std::string &path);
bool isRegularFile(const std::string &path);
bool isLink(const std::string &path);
PathSeq listDirectory(const std::string &path);
std::string resolvePath(const std::string &path);
bool isExtention(const std::string &path, const std::string &ext);
