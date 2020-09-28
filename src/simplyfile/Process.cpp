#include "Process.h"
#include <cstring>
#include <memory>
#include <stdexcept>

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/prctl.h>

namespace simplyfile 
{

namespace {

std::pair<FileDescriptor, FileDescriptor> pipe() {
    int fds[2] = {-1, -1};
    if (::pipe(fds)) {
        throw std::runtime_error("cannot open pipe");
    }
    return {FileDescriptor{fds[0]}, FileDescriptor{fds[1]}};
}

}

Process::Process(std::string_view command, std::vector<std::string> const& args)
{
    auto [p2c_r, p2c_w] = pipe();
    auto [c2p_r, c2p_w] = pipe();
    auto [c2p_e_r, c2p_e_w] = pipe();

    childPID = fork();
    if (childPID == 0) {
        // child
        p2c_w.close();
        c2p_r.close();
        c2p_e_r.close();

        prctl(PR_SET_PDEATHSIG, SIGTERM);

        dup2(p2c_r, STDIN_FILENO);
        dup2(c2p_w, STDOUT_FILENO);
        dup2(c2p_e_w, STDERR_FILENO);
        p2c_r.close();
        c2p_w.close();
        c2p_e_w.close();

        std::vector<char const*> argBuffer(args.size()+2, nullptr);
        argBuffer[0] = command.data();
        for (std::size_t i{0}; i < args.size(); ++i) {
            argBuffer[i+1] = args[i].data();
        }

        argBuffer[args.size()+1] = nullptr;
        char *const* args = const_cast<char *const*>(argBuffer.data());
        int r = execvp(command.data(), args);
        exit(r);
    } else {
        // parent
        stdin = std::move(p2c_w);
        stdout = std::move(c2p_r);
        stderr = std::move(c2p_e_r);
    }
}

Process::~Process() {
    if (std::current_exception()) { // if there is an exception in flight kill the subprocess
        kill(SIGKILL);
    }
    wait();
    stdin.close();
    stdout.close();
    stderr.close();
}

int Process::wait() {
    if (not childPID) { // dont wait pultiple times
        return 0;
    }
    int wstatus{0};
    pid_t w;
    do {
        w = waitpid(childPID, &wstatus, 0);
        if (w == -1) {
            throw std::runtime_error("waitpid returned -1 " + std::string(std::strerror(errno)));
        }
    } while (not WIFEXITED(wstatus) and not WIFSIGNALED(wstatus));
    childPID = 0;
    return WEXITSTATUS(wstatus);
}

int Process::kill(int signal) {
    return ::kill(childPID, signal);
}

}
