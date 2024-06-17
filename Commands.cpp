#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>
#include <climits>
#include <sys/types.h>
#include <sys/resource.h>
#include <csignal>
#include <regex>
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

ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}


void GetCurrDirCommand::execute() {
    char path[PATH_MAX];
    getcwd(path, PATH_MAX);
    cout << path << endl;
}

bool checkValid(const string& line){
    std::string str(line);
    std::string argument = str.substr(3);
    if (argument.find(' ') != string::npos) {
        return false;
    }
    return true;
}
void ChangeDirCommand ::execute() {
    ///// TO fix this
    string line = _trim(this->m_cmd);
    char former_path[PATH_MAX];
    getcwd(former_path, PATH_MAX);
    if(line == "cd") {
        this->m_lastPwd = string(former_path);
        return;
    }
    if(!checkValid(line)){
        //TODO: error
        std :: cerr << "error: cd: too many arguments\n";
        return;
    }
    if(line[3] == '-'){
        if(_trim(this->m_lastPwd).empty()){
            std :: cerr << "error: cd: OLDPWD not set\n";
            return;
        }
        new_path = this->m_lastPwd;
    }
    else{
        new_path = this->m_cmd.substr(3);
    }
    int res = chdir(new_path.c_str());
    if(res !=0){
        perror("smash error: chdir failed");
        return;
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
            str = str.substr(index+1);
        }
        return res;
}
void ExternalCommand :: execute(){
    std::vector<const char*> arguments;
    string line = _trim(this->m_cmd);
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
    cout <<"smash pid is " << m_pid << endl;
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


aliasCommand ::aliasCommand(const char *cmd_line, aliasCommand_DS *aliasDS) : BuiltInCommand(cmd_line) , m_aliasDS(aliasDS){}
void aliasCommand:: execute(){
    string line = _trim(string(m_cmd));
    if(line =="alias"){
        m_aliasDS->print_alias_command();
        return;
    }
    regex reg("^alias [a-zA-Z0-9_]+='[^']*'$");
    if(regex_match(line, reg) == false){
        throw std::invalid_argument("smash error: alias: invalid alias format");
    }
    string name  = line.substr(line.find_first_of(" ")+1, line.find_first_of("=")-(line.find_first_of(" ")+1));
    string command = m_cmd.substr(m_cmd.find_first_of("=") + 1);
    m_aliasDS->add_alias_command(name, command);//removing quotes from the command
}

unaliasCommand :: unaliasCommand(const char *cmd_line,aliasCommand_DS *aliasDS ) : BuiltInCommand(cmd_line) , m_aliasDS(aliasDS){}

void unaliasCommand :: execute(){
    char **args;
    int size = _parseCommandLine(m_cmd.c_str(), args);
    for(int i = 0 ; i < size; i++){
        try{
        m_aliasDS->remove_alias_command(string(args[i]));
        }catch(const std :: exception& e){
            for(int j = 0 ;j < size; j++){free(args[j]);}
            throw e;
        }
    }
    for(int j = 0 ;j < size; j++){free(args[j]);}
}


RedirectionCommand :: RedirectionCommand(const char *cmd_line){

}

void RedirectionCommand :: execute(){}
/////////////////////////////////////////

JobsList::JobsList(){}

std::ostream& operator<<(std::ostream& os, const JobsList::JobEntry& job){
    os << job.m_cmd;
    return os;
}

void JobsList :: addJob(Command *cmd, bool isStopped){
    unsigned int max_id = *(--m_max_ids.end());

    JobEntry job(isStopped, max_id+1, cmd);
    m_jobs.insert({max_id+1,job});
    m_max_ids.insert(max_id+1);
}

void JobsList :: printJobsList(){
    //TODO: Delete the finished jobs - shay
    for(auto i : m_jobs){
        cout << "[" << i.first  << "] " << i.second << endl;
    }
}

void JobsList :: killAllJobs(){
    for(auto i : m_jobs){
        kill();
    }
}

void JobsList :: removeFinishedJobs(){
//nitay
}

JobsList ::JobEntry* JobsList :: getJobById(int jobId){
    return &(m_jobs.find(unsigned int(jobId))->second);
}

void JobsList :: removeJobById(int jobId){
//SHAY
}

JobsList::JobEntry *getLastJob(int *lastJobId){
    //Nitay
}

JobsList::JobEntry *getLastStoppedJob(int *jobId){
//nitaY

}
// TODO: Add extra methods or modify exisitng ones as needed


JobsList :: JobEntry :: JobEntry(bool is_stopped, unsigned int id,Command* cmd) : m_id(id), m_cmd(cmd), m_is_finished(is_stopped) {}



/// @brief ///////////////////////////////
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
    }
    else if (firstWord.compare("alias") == 0) {
        return new aliasCommand(cmd_line, &m_aliasDS);
    }
    else if(m_aliasDS.checkInAlias(firstWord) == true){

        ////// TO handle alias command that is printed
        std :: string tmp = m_aliasDS.TranslateAlias(firstWord);
        if(tmp != ""){
            return this->CreateCommand(tmp.c_str());
        }
    }
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


///////////////////////////////////////////
void aliasCommand_DS :: add_alias_command(std :: string name, std::string command){
    if(this->checkInAlias(name) == true || count(SMASH_COMMANDS.begin(), SMASH_COMMANDS.end(), name) > 0){
        throw std::invalid_argument( "smash error: alias: " + name + " already exists or is a reserved command");
    }
    m_alias.push_back({name, command});
}
void aliasCommand_DS :: remove_alias_command(std :: string name){
    auto it = std::find_if(m_alias.begin(), m_alias.end(), [&name](const std::pair<std::string, std::string>& p) {
        return p.first == name;});
    if (it != m_alias.end()) {
        m_alias.erase(it);
    } else {
        throw std::invalid_argument( "smash error: unalias: " +name+" alias does not exist");
    }
}
void aliasCommand_DS :: print_alias_command(){
    for (auto i : m_alias){
        std ::cout << i.first << "=" << i.second << std::endl;
    }
}

bool aliasCommand_DS :: checkInAlias(std::string alias){
    auto it = std::find_if(m_alias.begin(), m_alias.end(), [&alias](const std::pair<std::string, std::string>& p) {
    return p.first == alias;});
    return (it != m_alias.end());
}
std::string aliasCommand_DS :: TranslateAlias(std::string alias){
    auto it = std::find_if(m_alias.begin(), m_alias.end(), [&alias](const std::pair<std::string, std::string>& p) {
    return p.first == alias;});
    return it->second;
}
