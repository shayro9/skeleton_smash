#include <iostream>
#include "signals.h"
//#include "Commands.h"
#include <sys/types.h>
#include <csignal>
#include <unistd.h>

using namespace std;

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    SmallShell &smash = SmallShell::getInstance();
    if(smash.isWaiting()){
        pid_t pid = smash.getWorkingPid();
        kill(pid, SIGKILL);
        cout << "smash: process "+ to_string(pid) + " was killed" << endl;
    }
}
