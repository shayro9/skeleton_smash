#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <limits.h>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h
GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
ChangePrompt::ChangePrompt(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char path[PATH_MAX];
    getcwd(path, PATH_MAX);
    cout << path << endl;
}


ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

bool checkValid(const char* line){
    std::string str(line);
    std::string argument = str.substr(3);
    if (argument.find(' ') != string::npos) {
        return false;
    }
    return true;
}
void ChangeDirCommand ::execute() {
    std :: string new_path;
    if(!checkValid(this->m_cmd)){
        //TODO: error
        std :: cerr << "error: cd: too many arguments\n";
        exit(1);
    }
    if(this->m_cmd[3] == '-'){
        if(this->m_lastPwd.empty()){
            std :: cerr << "error: cd: OLDPWD not set\n";
            exit(1);
        }
        new_path = this->m_lastPwd;
    }else{
        new_path = (this->m_cmd + 3);
    }
    char former_path[PATH_MAX];
    getcwd(former_path, PATH_MAX);
    int res = chdir(new_path.c_str());
    if(res !=0){
        perror("smash error: chdir failed");
        exit(1);
    }
    this->m_lastPwd = string(former_path);
}


ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}


vector<string> spllitStringByChar(string str, string delim) {
        vector<string> res;
        int index = str.find_first_of(delim);
        while(index != -1){
            index = str.find_first_of(delim);
            res.push_back(str.substr(0, index));
            str = str.substr(index+1, str.size());
        }
        return res;
}
void ExternalCommand :: execute(){
    std::vector<const char*> arguments;
    string line = this->m_cmd;
    string firstWord = line.substr(0, line.find_first_of(WHITESPACE));//?? why " \n"
    cout << "got here" << firstWord<< endl;
    if(firstWord.find(".") != string::npos){
        vector<string> tmp = spllitStringByChar(line, WHITESPACE);
        for(unsigned int  i= 0 ; i < tmp.size() ; i++){arguments.push_back(tmp[i].c_str());}
        arguments.push_back(nullptr);
    }
    else{
        arguments.push_back(("/bin/bash"));
        arguments.push_back(("-c"));
        arguments.push_back(line.c_str());
        arguments.push_back(nullptr);
    }

    int wstatus;
    pid_t pid = fork();
    if (pid == 0) {
        for(auto i : arguments){cout << i << endl;}
        execv(arguments[0], const_cast<char* const*>(arguments.data()));
    } else {
        waitpid(pid, &wstatus, 0);;
    }
}


void ShowPidCommand::execute() {
    pid_t pid;
    pid = getpid();
    cout <<"smash pid is " << pid << endl;
}

void ChangePrompt::execute() {
    string cmd_s = _trim(string(m_cmd));
    string prompt;
    int firstSpace = cmd_s.find_first_of(WHITESPACE);
    if (firstSpace > 0)
        prompt = cmd_s.substr( firstSpace + 1, cmd_s.find_first_of(" \n"));
    else
        prompt = "";
    SmallShell::getInstance().SetPrompt(prompt);
}



SmallShell::SmallShell() : m_prompt("smash> "){
// TODO: add your implementation

}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

std::string SmallShell::GetPrompt() {
    return m_prompt;
}

void SmallShell::SetPrompt(const string& prompt){
    if(prompt.size() > 0)
        m_prompt = prompt + "> ";
    else
        m_prompt = "smash> ";
}


/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    // For example:

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord.compare("chprompt") == 0) {
        return new ChangePrompt(cmd_line);
    else {
    return new ExternalCommand(cmd_line);

    }
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
     Command* cmd = CreateCommand(cmd_line);
     cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}
