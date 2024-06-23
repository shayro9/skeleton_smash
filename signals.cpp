#include <iostream>
#include "signals.h"
#include "Commands.h"
#include <sys/types.h>
#include <csignal>
#include <unistd.h>

using namespace std;

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    if(SmallShell::getInstance().isWaiting()){
        pid_t pid = getpid();
        kill(pid, SIGKILL);
        cout << "smash: process "+ to_string(pid) + " was killed." << endl;
    }
}
