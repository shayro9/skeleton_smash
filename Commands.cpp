#include <unistd.h>
#include <cstring>
#include <regex>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <climits>
#include <sys/types.h>
#include <sys/resource.h>
#include <csignal>
#include <sys/wait.h>
#include <iomanip>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include "Commands.h"
#include <dirent.h>
#include <sys/syscall.h>
#include <pwd.h>
#include <grp.h>

#define SYSCALL_CHECK(syscall, message) \
    do { \
        if ((syscall) == -1) { \
            perror(message.c_str()); \
            return; \
        } \
    } while (0)


using namespace std;

extern Command* WatchCommand::m_command;
extern bool WatchCommand::m_isBg;
extern std :: string ChangeDirCommand :: m_lastPwd;
extern const vector<string> SMASH_COMMANDS = {"pwd", "cd", "chprompt", "showpid"};
const std::string WHITESPACE = " \n\r\t\f\v";
const int STDIN = 0;
const int STDOUT = 1;
const int STDERR = 2;
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
string _StringremoveBackgroundSign(const char *cmd_line) {
    string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return str;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return str;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    str[idx] = ' ';
    // truncate the command line string up to the last non-space character
    return _rtrim(str);
}
string insertErrorMessage(const string msg){
    return ("smash error: " + msg + " failed");
}

string Command::GetLine() const {
    return m_cmd;
}
std::ostream& operator<<(std::ostream& os, const Command &cmd){
    os << cmd.m_cmd;
    return os;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(_StringremoveBackgroundSign(cmd_line)){
    //string str= cmd_line;
    //this->m_cmd = str;
}
GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
ChangePrompt::ChangePrompt(const char *cmd_line) : BuiltInCommand(cmd_line) {
    string cmd_s = _trim(string(m_cmd));
    vector<string> args;
    int args_num = _parseCommandLine(cmd_s.c_str(), args);
    if(args_num == 1){
        m_prompt = "smash";
    }
    else {
        m_prompt = args[1];
    }
}
ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), m_jobs(jobs){}
ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), m_jobs(jobs) {
    string cmd_s = _trim(string(m_cmd));
    vector<string> args;
    int args_num = _parseCommandLine(cmd_s.c_str(), args);
    if (args_num == 1) {
        if (m_jobs->isEmpty())
            throw out_of_range("smash error: fg: jobs list is empty");
        m_job_id = *(--jobs->m_max_ids.end());
    }
    else if (args_num > 2) {
        throw invalid_argument("smash error: fg: invalid arguments");
    }
    else{
        try{
            m_job_id = stoi(args[1]);
            if(m_job_id < 0){
				throw invalid_argument("smash error: fg: invalid arguments");
			}
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
GetUserCommand::GetUserCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    string cmd_s = _trim(string(m_cmd));
    vector<string> args;
    int args_num = _parseCommandLine(cmd_s.c_str(), args);
    if(args_num == 2){
        m_targetPid = stoi(args[1]);
    }
    else{
        throw invalid_argument("smash error: getuser: too many arguments");
    }
}
WatchCommand::WatchCommand(const char *cmd_line) : Command(cmd_line){
    string cmd_s = _trim(string(m_cmd));
    vector<string> args;
    int args_num = _parseCommandLine(cmd_s.c_str(), args);
    if(args_num == 1){
        throw invalid_argument("smash error: watch: invalid interval");
    }
    int i = 2;
    try {
        m_interval = stoi(args[1]);
    }
    catch (...){
        m_interval = 2;
        i = 1;
    }
    if(i == 2 && args_num == 2){
        throw invalid_argument("smash error: watch: command not specified");
    }
    if(m_interval <= 0){
        throw invalid_argument("smash error: watch: invalid interval");
    }
    m_isBg = args.back().find_first_of('&') != string::npos;
    string line;
    line = cmd_s.substr(cmd_s.find(args[i]));
    SmallShell& shell = SmallShell::getInstance();
    m_command = shell.CreateCommand(line.c_str());
}

void GetCurrDirCommand::execute() {
    char path[PATH_MAX];
    if(getcwd(path, PATH_MAX) == nullptr)
    {
        perror("smash error: getcwd failed");
    }
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
    string cmd_s = _trim(string(m_cmd));
    vector<string> args;
    string new_path;
    int args_num = _parseCommandLine(cmd_s.c_str(), args);
    if(args_num > 2 ){
        //TODO: error
        throw runtime_error("error: cd: too many arguments");
    }
    if(args_num > 1 && args[1] == "-"){
        if(_trim(this->m_lastPwd).empty()){
            throw runtime_error("error: cd: OLDPWD not set");
        }
        new_path = this->m_lastPwd;
    }else{
        new_path = args_num >1 ? args[1] : "";
    }
    char former_path[PATH_MAX];
    if(getcwd(former_path, PATH_MAX) == NULL){
        perror(insertErrorMessage("getcwd").c_str());
        return;
    }
    if(args_num == 1){
        this->m_lastPwd = string(former_path);
        return;
    }
    int res;
    SYSCALL_CHECK((res = chdir(new_path.c_str())), insertErrorMessage("chdir") );
    this->m_lastPwd = string(former_path);
}
void ShowPidCommand::execute() {
    pid_t pid;
    pid = getpid();
    cout <<"smash pid is " << pid << endl;
}
void ChangePrompt::execute() {
    SmallShell::getInstance().SetPrompt(m_prompt);
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
    pid_t workingPid = m_jobs->getJobById((m_job_id))->Getpid();
    int status;
    smash.setWorkingPid(workingPid);
    cout << cmd->GetLine() << " " << workingPid << endl;
    m_jobs->removeJobById(m_job_id);
    if(waitpid(workingPid, &status, 0) == -1){
        perror("smash error: waitpid failed");
    }
    smash.setWorkingPid(-1);
}
void KillCommand::execute() {
    pid_t pid = m_jobs->getJobById(m_jobId)->Getpid();
    if(kill(pid, m_signum) == -1){
        perror("smash error: kill failed");
    }
    if(m_signum == 9 || m_signum == 3 || m_signum == 1)
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
    if(lstat(path, &st) == -1){
        perror("smash error: lstat failed");
    }
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
void printKey(string key, set<string> values){
    cout << key << ": ";
    for(const auto& file : values){
        cout << file;
        if(values.find(file) != --values.end())
            cout << ", ";
    }
    cout << endl;
}
void ListDirCommand::execute() {
    int opened = open(m_path.c_str(), O_RDONLY | O_DIRECTORY);
    if(opened == -1){
        perror("smash error: open failed");
        return;
    }

    const int maxRead = 100;
    char buffer[maxRead];
    ssize_t bytesRead;
    map<string ,set<string>> filesMap;
    filesMap.insert( pair<string ,set<string>>("link", set<string>()));
    filesMap.insert(pair<string ,set<string>>("directory", set<string>()));
    filesMap.insert( pair<string ,set<string>>("file", set<string>()));


    while ((bytesRead = syscall(SYS_getdents, opened, buffer, maxRead)) > 0) {
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
    if(bytesRead == -1){
        perror("smash error: getdents failed");
    }


    if(!filesMap["file"].empty()) printKey("file", filesMap["file"]);
    if(!filesMap["directory"].empty()) printKey("directory", filesMap["directory"]);
    if(!filesMap["link"].empty()) printKey("link", filesMap["link"]);

}
void GetUserCommand::execute() {
    string procPath = "/proc/" + to_string(m_targetPid) + "/status";
    struct stat procStat;
    if(stat(procPath.c_str(), &procStat) == -1) { //TODO perror?????
        throw invalid_argument("smash error: getuser: process " + to_string(m_targetPid) + " does not exist");
    }

    uid_t uid = procStat.st_uid;
    struct passwd *pw = getpwuid(uid);
    if(pw == nullptr) {
        throw invalid_argument("smash error: getuser: process " + to_string(m_targetPid) + " does not exist");
    }
    gid_t grp = procStat.st_gid;
    struct group *grp_entry = getgrgid(grp);
    if(grp_entry == nullptr){
        perror("smash error: getgrgid failed");
    }

    string userName, groupName;
    groupName = grp_entry->gr_name;
    userName = pw->pw_name;

    cout << "User: " << userName << endl << "Group: " << groupName << endl;
}
void WatchCommand::signalHandler(int sig_num) {
    if(!m_isBg)
        system("clear");
    m_command->execute();
}
void WatchCommand::execute() {
	int wstatus;
    pid_t child_pid = fork();
    if(child_pid == -1){
        perror("smash error: fork failed");
    }
    if (child_pid == 0) {
		if(setpgrp() == -1){
            perror("smash error: setpqrp failed");
        }
        if(signal(SIGALRM, signalHandler) == SIG_ERR){
            perror("smash error: signal failed");
        }
        int devNull = open("/dev/null", O_WRONLY);
        dup2(devNull, STDOUT_FILENO);
        close(devNull);

        struct itimerval timer;
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = 10;
        timer.it_interval.tv_sec = m_interval;
        timer.it_interval.tv_usec = 0;

		if(setitimer(ITIMER_REAL, &timer, nullptr) == -1){
            perror("smash error: setitimer failed");
        }
		
        while (true) {
            pause();
        }
    } else {
        if(!m_isBg) {
            SmallShell::getInstance().setWorkingPid(child_pid);
            if (waitpid(child_pid, &wstatus, 0) == -1) {
                perror("smash error: waitpid failed");
            }
            SmallShell::getInstance().setWorkingPid(-1);
        }
        else {
            SmallShell::getInstance().addJob(this, child_pid);
        }
    }

}

/////////////////////////////////////////

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
ExternalCommand::ExternalCommand(const char *cmd_line, string org_line) : Command(cmd_line), m_org_word(org_line) {}
string ExternalCommand :: GetLine() const 
{
    return this->m_org_word;
}
void ExternalCommand :: execute(){
    std::vector<const char*> arguments;
    string line = _trim(this->m_cmd);
    //string firstWord = line.substr(0, line.find_first_of(WHITESPACE));//?? why " \n"
    _removeBackgroundSign(&line[0]);

//TO DO : make a call to the function that is described in the notes
    if(line.find("?") != string::npos || line.find("*") != string::npos){
        arguments.push_back(("/bin/bash"));
        arguments.push_back(("-c"));
        arguments.push_back(line.c_str());
        arguments.push_back(nullptr);
    }else{
        vector<string> tmp;
        _parseCommandLine(line.c_str(), tmp);
        for(unsigned int  i= 0 ; i < tmp.size() ; i++){arguments.push_back(tmp[i].c_str());}
        arguments.push_back(nullptr);
    }
    int wstatus;
    pid_t pid = fork();
    if(pid == -1){
        perror("smash error: fork failed");
    }
    if(_isBackgroundComamnd(this->m_cmd) == true && pid > 0){
        SmallShell::getInstance().addJob(this, pid);
    }
    SmallShell &smash = SmallShell::getInstance();
    if (pid == 0) {
        setpgrp();
        if(execvp(arguments[0], const_cast<char* const*>(arguments.data())) == -1)
        {
            perror("smash error: execvp failed");
            exit(1);
        }
    } else if(_isBackgroundComamnd(this->m_cmd) == false){
        smash.setWorkingPid(pid);
        if(waitpid(pid, &wstatus, 0) == -1){
            perror("smash error: waitpid failed");
        }
        smash.setWorkingPid(-1);
    }
}

aliasCommand ::aliasCommand(const char *cmd_line, aliasCommand_DS *aliasDS) : BuiltInCommand(cmd_line) , m_aliasDS(aliasDS){}
void aliasCommand:: execute(){
    string cmd_s = _trim(string(m_cmd));
    vector<string> args;
    string new_path;
    int args_num = _parseCommandLine(cmd_s.c_str(), args);
    string line = _trim(string(m_cmd));
    if(args_num == 1 && args[0] =="alias"){
        m_aliasDS->print_alias_command();
        return;
    }
    //string name  = line.substr(line.find_first_of(" ")+1, line.find_first_of("=")-(line.find_first_of(" ")+1));
    string name  = args[1].substr(0,args[1].find_first_of(WHITESPACE));
    if(this->m_aliasDS->checkInAlias(name) == true || count(SMASH_COMMANDS.begin(), SMASH_COMMANDS.end(), name) > 0){
        throw std::invalid_argument( "smash error: alias: " + name + " already exists or is a reserved command");
    }
    regex reg("^alias [a-zA-Z0-9_]+='[^']*'$");
    if(regex_match(line, reg) == false){
        throw std::invalid_argument("smash error: alias: invalid alias format");
    }
    string command = args[1].substr(args[1].find_first_of("=") + 1);
    m_aliasDS->add_alias_command(name, command);// without removing quotes from the command
}

unaliasCommand :: unaliasCommand(const char *cmd_line,aliasCommand_DS *aliasDS ) : BuiltInCommand(cmd_line) , m_aliasDS(aliasDS){}

void unaliasCommand :: execute(){
    vector<string> args;
    int size = _parseCommandLine(m_cmd.c_str(), args);
    for(int i = 1 ; i < size; i++){
        //try{
        m_aliasDS->remove_alias_command(string(args[i]));
        //}catch(const std :: exception& e){
          //  for(int j = 0 ;j < size; j++){free(args[j]);}
            //throw e;
        //}
    }
    //for(int j = 0 ;j < size; j++){free(args[j]);}
}


RedirectionCommand :: RedirectionCommand(const char *cmd_line) : Command(cmd_line) {}


void RedirectionCommand :: execute(){
    pid_t pid = fork();
    if(pid == -1){
        perror("smash error: fork failed");
    }
    if(pid == 0){
        std ::string line = _trim(_StringremoveBackgroundSign(this->m_cmd.c_str()));
        string file_name = _trim(line.substr(line.find_last_of(">")+1));
        int file = (line.find(">>") != string::npos) ? open(file_name.c_str(), O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR) :open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (file < 0) {
            perror("smash error: open failed");
        }
/*        int former_std_fd = dup(STDOUT);
        if(former_std_fd < 0) {
            close(file);
            throw std::runtime_error("smash error: dup failed");
        }*/
        if (dup2(file, STDOUT) < 0) {
            close(file);
            perror("smash error: dup2 failed");
        }
        SmallShell::getInstance().CreateCommand(line.substr(0, line.find_first_of('>') - 1).c_str())->execute();
        close(STDOUT);
        /*if (dup2(m_std_fd, STDOUT) < 0) {
            throw std::runtime_error("smash error: dup2 failed");
        }*/
        exit(0);
    }else{
        int status;
        if(waitpid(pid, &status, 0) == -1){
            perror("smash error: waitpid failed");
        }
    }
}


PipeCommand :: PipeCommand(const char *cmd_line) : Command(cmd_line){}

void PipeCommand :: execute(){
    pid_t pid_number, pid;
    if(( pid_number = fork()) == 0){
        vector<string> args;
        _parseCommandLine(this->m_cmd.c_str(), args);
        bool after_pipe_symbol = false;
        int stderr_or_stdout = STDOUT;
        string first_command = "", last_command = "";
        for(unsigned int i=0; i<args.size(); i++){
            if(args[i] == "|" || args[i] == "|&"){
                after_pipe_symbol = true;
                if(args[i] == "|&"){
                    stderr_or_stdout = STDERR;
                }
                continue;
            }
            if(after_pipe_symbol == false){
                first_command += args[i] + " ";
            }else{
                last_command += args[i] + " ";
            }
        }
        if(args.size() < 3 || first_command == "" || last_command == ""){
            throw std::runtime_error("smash error: unvalid pipe command");
        }
        int my_pipe[2];
        if(pipe(my_pipe) == -1){
            perror("smash error: pipe failed");
        }

        if ((pid = fork()) == 0) { // son
            close(my_pipe[0]);
            dup2(my_pipe[1], stderr_or_stdout); 
            close(my_pipe[1]); 
            SmallShell::getInstance().executeCommand(first_command.c_str());
            exit(0);
            }
        else{ // father
            if(pid == -1){
                perror("smash error: fork failed");
            }
            close(my_pipe[1]);          // Close the write end of the pipe
            dup2(my_pipe[0], STDIN);   // Redirect stdin to the read end of the pipe
            close(my_pipe[0]);
            SmallShell::getInstance().executeCommand(last_command.c_str());
            exit(0);
        }
    }else{
        if(pid_number == -1){
            perror("smash error: fork failed");
        }
        int status;
        if(waitpid(pid_number, &status, 0) == -1){
            perror("smash error: waitpid failed");
        }
    }
}
/////////////////////////////////////////

JobsList::JobsList(){}


std::ostream& operator<<(std::ostream& os, const JobsList::JobEntry& job){
    os << job.m_cmd->GetLine();
    return os;
}

void JobsList :: addJob(Command *cmd, pid_t pid, bool isStopped){

    unsigned int max_id = 0;
    if(!m_max_ids.empty())
        max_id = *(--m_max_ids.end());

    JobEntry job(isStopped, max_id+1, cmd, pid);
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
        if(kill(temp_job->Getpid(), SIGKILL) == -1){
            perror("smash error: kill failed");
        }
    }
}

void JobsList :: removeFinishedJobs(){
    vector<int> ids;
    std::copy(m_max_ids.begin(), m_max_ids.end(), back_inserter(ids));
    int status;
    for (auto i : ids){
        auto it = m_jobs.find(i);
        if (it != m_jobs.end()){
            int wait_num = waitpid(it->second.Getpid(), &status, WNOHANG);
            if(wait_num == -1)
                perror("smash error: waitpid failed");
            else if(wait_num > 0)
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

JobsList :: JobEntry :: JobEntry(bool is_stopped, unsigned int id,Command* cmd, pid_t pid) : m_id(id), m_is_finished(is_stopped), m_cmd(cmd), m_pid(pid) {}


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
SmallShell::SmallShell() : m_prompt("smash> "), m_fgPid(-1){

}

SmallShell::~SmallShell() {
}

std::string SmallShell::GetPrompt() {
    return m_prompt;
}

void SmallShell::SetPrompt(const string& prompt){
    m_prompt = prompt + "> ";
}

void SmallShell::addJob(Command* cmd, pid_t pid) {
    m_jobsList.addJob(cmd, pid);
}


/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string original_line(cmd_line);
    char* str = new char[cmd_s.size() + 1];
    for(unsigned int i = 0; i < cmd_s.size(); i++){str[i] = cmd_s[i];}
    str[cmd_s.size()] = '\0';
    _removeBackgroundSign(str);
    cmd_s = str;
    delete[] str;
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));
    //string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if(this->m_aliasDS.checkInAlias(firstWord)){
        string tmp = this->m_aliasDS.TranslateAlias(firstWord);
        tmp = tmp.substr(1, tmp.size()-2);
        cmd_s = string(cmd_line);
        cmd_line = (cmd_s.find_first_of(" \n") != string::npos) ? (tmp + cmd_s.substr(cmd_s.find_first_of(" \n"))).c_str() : tmp.c_str();
        cmd_s = _trim(string(cmd_line));
        str = new char[cmd_s.size() + 1];
        for(unsigned int i = 0; i < cmd_s.size(); i++){str[i] = cmd_s[i];}
        str[cmd_s.size()] = '\0';
        _removeBackgroundSign(str);
        cmd_s = str;
        delete[] str;
        firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));
    }
    if (firstWord == "cd") {
        return new ChangeDirCommand(cmd_line);
    }
    else if (firstWord == "chprompt") {
        return new ChangePrompt(cmd_line);
    }
    else if (firstWord == "fg") {
        return new ForegroundCommand(cmd_line, &m_jobsList);
    }
    else if (firstWord == "quit") {
        return new QuitCommand(cmd_line, &m_jobsList);
    }
    else if (firstWord == "watch") {
        return new WatchCommand(cmd_line);
    }
    if(cmd_s.find("|") != string::npos){
        return new PipeCommand(cmd_line);
    }
    if(cmd_s.find(">") != string::npos){
        return new RedirectionCommand(cmd_line);
    }
    if (firstWord == "pwd") {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord == "showpid") {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord == "jobs") {
        return new JobsCommand(cmd_line, &m_jobsList);
    }
    else if (firstWord == "kill") {
        return new KillCommand(cmd_line, &m_jobsList);
    }
    else if (firstWord == "listdir") {
        return new ListDirCommand(cmd_line);
    }
    else if (firstWord == "getuser") {
        return new GetUserCommand(cmd_line);
    }
    else if (firstWord == "alias"){
        return new aliasCommand(cmd_line,&m_aliasDS);
    }
    else if (firstWord == "unalias"){
        return new unaliasCommand(cmd_line,&m_aliasDS);
    }
    else {
        return new ExternalCommand(cmd_line,original_line);
    }
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    if(_trim(string(cmd_line)).empty()){
        return;
    }
    try {
        m_jobsList.removeFinishedJobs();
        Command* cmd = CreateCommand(cmd_line);
        cmd->execute();
    }
    catch (const exception& e){
        cerr << e.what() << endl;
    }
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
void aliasCommand_DS :: add_alias_command(std :: string name, std::string command){
    /*if(this->checkInAlias(name) == true || count(SMASH_COMMANDS.begin(), SMASH_COMMANDS.end(), name) > 0){
        throw std::invalid_argument( "smash error: alias: " + name + " already exists or is a reserved command");
    }*/ 
   //Already checked in the function
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
