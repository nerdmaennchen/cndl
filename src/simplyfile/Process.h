#include "FileDescriptor.h"
#include <unistd.h>
#include <string_view>

namespace simplyfile {

struct Process {
    Process(std::string_view command, std::vector<std::string> const& args={});
    Process(Process const&) = delete;
    Process& operator=(Process const&) = delete;
    Process(Process&&) = default;
    Process& operator=(Process&&) = default;

    ~Process();

    int wait();
    int kill(int signal);

    FileDescriptor stdin;
    FileDescriptor stdout;
    FileDescriptor stderr;
private:
    pid_t childPID {0};
};

}
