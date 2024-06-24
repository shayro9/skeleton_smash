#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>
#include <climits>
#include <sys/types.h>
#include <csignal>
#include <sys/resource.h>
#include <regex>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iomanip>
#include "Commands.h"
#include <dirent.h>
#include <sys/syscall.h>

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

int _parseCommandLine(const char *cmd_line, vector<string>& args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args.push_back(s);
        ++i;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(string cmd_line) {
    return cmd_line[cmd_line.find_last_not_of(WHITESPACE)] == '&';
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
    cmd_line[str.find_last_not_of(WHITESPACE, idx)] = 0;
}

// TODO: Add your implementation for classes in Commands.h
string Command::GetLine() const {
    return m_cmd;
}
std::ostream& operator<<(std::ostream& os, const Command &cmd){
    os << cmd.m_cmd;
    return os;
}


GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
ChangePrompt::ChangePrompt(const char *cmd_line) : BuiltInCommand(cmd_line) {}
//ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), m_jobs(jobs){}
ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), m_jobs(jobs) {
    string cmd_s = _trim(string(m_cmd));
    vector<string> args;
    int args_num = _parseCommandLine(cmd_s.c_str(), args);
    if (args_num > 2)
        throw invalid_argument("smash error: fg: invalid arguments");
    if (args_num == 1) {
        if (m_jobs->isEmpty())
            throw out_of_range("smash error: fg: jobs list is empty");
        m_job_id = *(--jobs->m_max_ids.end());
    }
    else{
        try{
            m_job_id = stoi(args[1]);
        }
        catch (...){
            throw invalid_argument("smash error: fg: invalid arguments");
        }
    }
}
KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){
    string cmd_s = _trim(string(m_cmd));
    vector<string> args;
    int args_num = _parseCommandLine(cmd_s.c_str(), args);

    if(args_num - 1 != 2)
        throw invalid_argument("smash error: kill: invalid arguments");
    if(args[1][0] != '-')
        throw invalid_argument("smash error: kill: invalid arguments");
    try {
        m_signum = stoi(args[1].substr(1));
        m_jobId = stoi(args[2]);
    }
    catch (...) {
        throw invalid_argument("smash error: kill: invalid arguments");
    }
    m_jobs = jobs;
    JobsList::JobEntry* job = jobs->getJobById(m_jobId);
    if (job == nullptr){
        throw invalid_argument("smash error: kill: job-id " + to_string(m_jobId) + " does not exist");
    }
}
QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), m_jobs(jobs){
    string cmd_s = _trim(string(m_cmd));
    vector<string> args;
    _parseCommandLine(cmd_s.c_str(), args);
    m_2kill = false;
    for(string& arg : args)
        if (arg == "kill") {
            m_2kill = true;
            break;
        }
}
ListDirCommand::ListDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    string cmd_s = _trim(string(m_cmd));
    vector<string> args;
    int args_num = _parseCommandLine(cmd_s.c_str(), args);
    if(args_num == 1){
        char buffer[PATH_MAX];
        getcwd(buffer, PATH_MAX);
        m_path = buffer;
    }
    else if(args_num == 2){
        m_path = args[1];
    }
    else{
        throw invalid_argument("smash error: listdir: too many arguments");
    }
}

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
//    ///// TO fix this
//    std :: string new_path;
//    string line = _trim(this->m_cmd);
//    if(!checkValid(line)){
//        //TODO: error
//        std :: cerr << "error: cd: too many arguments\n";
//        return;
//    }
//    if(line[3] == '-'){
//        if(_trim(this->m_lastPwd).empty()){
//            std :: cerr << "error: cd: OLDPWD not set\n";
//            return;
//        }
//        new_path = this->m_lastPwd;
//    }else{
//        new_path = this->m_cmd.substr(3, this->m_cmd.size());
//    }
//    char former_path[PATH_MAX];
//    getcwd(former_path, PATH_MAX);
//    int res = chdir(new_path.c_str());
//    if(res !=0){
//        perror("smash error: chdir failed");
//        return;
//    }
//    this->m_lastPwd = string(former_path);
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
void JobsCommand::execute() {
    m_jobs->printJobsList();
}
void ForegroundCommand::execute() {
    JobsList::JobEntry* job = m_jobs->getJobById((m_job_id));
    if(job == nullptr){
        throw invalid_argument("smash error: fg: job-id " + to_string(m_job_id) + " does not exist");
    }
    SmallShell &smash = SmallShell::getInstance();
    Command* cmd = job->GetCommand();
    pid_t workingPid = job->Getpid();
    cmd->execute();
    int status;

    smash.setWorkingPid(workingPid);
    waitpid(workingPid, &status, 0);
    cout << cmd->GetLine() << endl;
    smash.setWorkingPid(-1);
}
void KillCommand::execute() {
    pid_t pid = m_jobs->getJobById(m_jobId)->Getpid();
    if(kill(pid, m_signum)){
        //TODO use perror failed
    }
    if(m_signum == 9 || m_signum == 3 || m_signum == 1) //TODO: need to check if the signal succeeded? sigs(1 && 3)
        m_jobs->removeJobById(m_jobId);

    cout << "signal number " << m_signum << " was sent to pid " << pid << endl;
}
void QuitCommand::execute() {
    if (m_2kill) {
        m_jobs->killAllJobs();
    }
    exit(0);
}
void sortFiles(map<string, set<string>> &map, const char* path, string fileName){
    struct stat st;
    lstat(path, &st);
    if(S_ISREG(st.st_mode)){
        map["file"].insert(fileName);
    }
    else if(S_ISDIR(st.st_mode)){
        map["directory"].insert(fileName);
    }
    else if(S_ISLNK(st.st_mode)){
        map["link"].insert(fileName);
    }
}
void ListDirCommand::execute() {
    int opened = open(m_path.c_str(), O_RDONLY | O_DIRECTORY);
    if(opened == -1){
        perror("smash error: could not open directory");
        return;
    }

    const int maxRead = 100;
    char buffer[maxRead];
    ssize_t bytesRead;
    map<string ,set<string>> filesMap;
    filesMap["file"] = set<string>();
    filesMap["directory"] = set<string>();
    filesMap["link"] = set<string>();

    if ((bytesRead = syscall(SYS_getdents, opened, buffer, maxRead)) > 0) {
        int offset = 0;
        while (offset < bytesRead) {
            auto* entry = (struct linux_dirent*)(buffer + offset);
            string fileName = entry->d_name;
            if (entry->d_ino != 0 && fileName != "." && fileName != "..") {
                string fullPath = m_path + "/" + fileName;
                sortFiles(filesMap, fullPath.c_str(), fileName);
            }
            offset += entry->d_reclen;
        }
    }

    for (const auto& fileType : filesMap) {
        if(fileType.second.empty()) continue;
        cout << fileType.first << ": ";
        for(const auto& file : fileType.second){
            cout << file << ", ";
        }
        cout << endl;
    }
}

/////////////////////////////////////////

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
    string line = _trim(this->m_cmd);
    string firstWord = line.substr(0, line.find_first_of(WHITESPACE));

    if(firstWord.find('.') != string::npos){
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
    SmallShell &smash = SmallShell::getInstance();
    if (pid == 0) {
        setpgrp();
        for(auto i : arguments){cout << i << endl;}
        execv(arguments[0], const_cast<char* const*>(arguments.data()));
    } else {
        smash.setWorkingPid(pid);
        waitpid(pid, &wstatus, 0);
        smash.setWorkingPid(-1);
    }
}
 
//aliasCommand ::aliasCommand(const char *cmd_line, aliasCommand_DS *aliasDS) : BuiltInCommand(cmd_line) , m_aliasDS(aliasDS){}
//void aliasCommand:: execute(){
//    string line = _trim(string(m_cmd));
//    if(line =="alias"){
//        m_aliasDS->print_alias_command();
//        return;
//    }
//    regex reg("^alias [a-zA-Z0-9_]+='[^']*'$");
//    if(regex_match(line, reg) == false){
//        throw std::invalid_argument("smash error: alias: invalid alias format");
//    }
//    string name  = line.substr(line.find_first_of(" ")+1, line.find_first_of("=")-(line.find_first_of(" ")+1));
//    string command = m_cmd.substr(m_cmd.find_first_of("=") + 1);
//    m_aliasDS->add_alias_command(name, command);//removing quotes from the command
//}
//
//unaliasCommand :: unaliasCommand(const char *cmd_line,aliasCommand_DS *aliasDS ) : BuiltInCommand(cmd_line) , m_aliasDS(aliasDS){}
//
//void unaliasCommand :: execute(){
//    vector<string> args;
//    int size = _parseCommandLine(m_cmd.c_str(), args);
//    for(int i = 0 ; i < size; i++){
//        try{
//        m_aliasDS->remove_alias_command(string(args[i]));
//        }catch(const std :: exception& e){
//            throw e;
//        }
//    }
//}


//RedirectionCommand :: RedirectionCommand(const char *cmd_line){
//    int former_std_fd = dup(STDOUT)
//    int file = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
//}
//
//void RedirectionCommand :: execute(){}
/////////////////////////////////////////

JobsList::JobsList(){}

void JobsList :: addJob(Command *cmd, bool isStopped){
    unsigned int max_id = 0;
    if(!m_max_ids.empty())
        max_id = *(--m_max_ids.end());

    JobEntry job(isStopped, max_id+1, cmd);
    m_jobs.insert({max_id+1,job});
    m_max_ids.insert(max_id+1);
}

void JobsList :: printJobsList(){
    removeFinishedJobs();
    for(auto i : m_jobs){
        cout << "[" << i.first  << "] " << i.second << endl;
    }
}

void JobsList :: killAllJobs(){
    removeFinishedJobs();
    cout << "smash: sending SIGKILL signal to "+ to_string(m_jobs.size()) +" jobs:" << endl;
    vector<int> ids;
    std::copy(m_max_ids.begin(), m_max_ids.end(), back_inserter(ids));
    for (auto i : ids){
        JobEntry* temp_job = &m_jobs.find(i)->second;
        Command* temp_cmd = temp_job->GetCommand();
        cout << temp_job->Getpid() << ": " << temp_cmd->GetLine() << endl;
        kill(temp_job->Getpid(), SIGKILL);
    }
}

void JobsList :: removeFinishedJobs(){
    vector<int> ids;
    std::copy(m_max_ids.begin(), m_max_ids.end(), back_inserter(ids));
    int status;
    for (auto i : ids){
        auto it = m_jobs.find(i);
        if (it != m_jobs.end()){
            if(waitpid(it->second.Getpid(), &status, WNOHANG))
                removeJobById(i);
        }
    }
}

JobsList ::JobEntry* JobsList :: getJobById(int jobId){
    auto it = m_jobs.find((unsigned int) jobId);
    if(it == m_jobs.end()){
        return nullptr;
    }
    return &it->second;
}

void JobsList :: removeJobById(int jobId){
    m_jobs.erase(jobId);
    m_max_ids.erase(jobId);
}

JobsList::JobEntry *getLastJob(int *lastJobId){
    return nullptr;
}

JobsList::JobEntry *getLastStoppedJob(int *jobId){
    return nullptr;
}

bool JobsList::isEmpty() const {
    return m_jobs.empty();
}

JobsList :: JobEntry :: JobEntry(bool is_stopped, unsigned int id,Command* cmd) : m_id(id), m_is_finished(is_stopped), m_cmd(cmd) {}

std::ostream& operator<<(std::ostream& os, const JobsList::JobEntry& job){
    os << job.m_cmd->GetLine();
    return os;
}

Command *JobsList::JobEntry::GetCommand() const {
    return m_cmd;
}

bool JobsList::JobEntry::isFinished() const {
    return m_is_finished;
}

void JobsList::JobEntry::Done() {
    m_is_finished = true;
}

pid_t JobsList::JobEntry::Getpid() const {
    return m_pid;
}


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
    if(!prompt.empty())
        m_prompt = prompt + "> ";
    else
        m_prompt = "smash> ";
}

void SmallShell::addJob(Command* cmd) {
    m_jobsList.addJob(cmd);
}


/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    _removeBackgroundSign(&cmd_s[0]);
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord == "pwd") {
        return new GetCurrDirCommand(cmd_line);
    }
//    else if (firstWord == "cd") {
//        return new ChangeDirCommand(cmd_line);
//    }
    else if (firstWord == "showpid") {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord == "chprompt") {
        return new ChangePrompt(cmd_line);
    }
    else if (firstWord == "jobs") {
        return new JobsCommand(cmd_line, &m_jobsList);
    }
    else if (firstWord == "fg") {
        return new ForegroundCommand(cmd_line, &m_jobsList);
    }
    else if (firstWord == "kill") {
        return new KillCommand(cmd_line, &m_jobsList);
    }
    else if (firstWord == "quit") {
        return new QuitCommand(cmd_line, &m_jobsList);
    }
    else if (firstWord == "listdir") {
        return new ListDirCommand(cmd_line);
    }
    else {
        return new ExternalCommand(cmd_line);
    }
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    try {
        Command* cmd = CreateCommand(cmd_line);
        cmd->execute();
    }
    catch (const exception& e){
        cout << e.what() << endl;
    }
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

bool SmallShell::isWaiting() const {
    return m_fgPid != -1;
}

void SmallShell::setWorkingPid(pid_t pid) {
    m_fgPid = pid;
}

pid_t SmallShell::getWorkingPid() const {
    return m_fgPid;
}
