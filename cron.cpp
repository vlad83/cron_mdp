#include <iostream>

#include <unistd.h>

#include "Cron.h"

namespace {

void help(const char *argv0, const char *message = nullptr)
{
    if(message) std::cout << "WARNING: " << message << '\n';

    std::cout
        << argv0
        << " -a broker_address"
        << " -p path"
        << std::endl;
}

} /* namespace */

int main(int argc, char *const argv[])
{
    std::string brokerAddress;
    std::string path;

    for(char c; -1 != (c = ::getopt(argc, argv, "ha:p:"));)
    {
        switch(c)
        {
            case 'h':
                help(argv[0]);
                return EXIT_SUCCESS;
                break;
            case 'a':
                brokerAddress = optarg ? optarg : "";
                break;
            case 'p':
                path = optarg ? optarg : "";
                break;
            case ':':
            case '?':
            default:
                help(argv[0], "geopt() failure");
                return EXIT_FAILURE;
                break;
        }
    }

    if(
        brokerAddress.empty()
        || path.empty())
    {
        help(argv[0], "missing/invalid required arguments");
        return EXIT_FAILURE;
    }

    try
    {
        using namespace cron;

        Cron cron(brokerAddress, path);
        cron.exec();
    }
    catch(const std::exception &except)
    {
        std::cerr << "std exception " << except.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch(...)
    {
        std::cerr << "unsupported exception" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
