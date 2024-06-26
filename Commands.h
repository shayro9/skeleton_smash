#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_


#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#include <map>
#include <vector>
#include <string>
#include <set>

using namespace std;
//TODO ?????
//static vector<string> SMASH_COMMANDS = {"pwd", "cd", "chprompt", "showpid"};

struct linux_dirent{
    unsigned long   d_ino;
    off_t           d_off;
    unsigned short  d_reclen;
    char            d_name[];
};

class Command {
protected:
    const std::string m_cmd;
public:
    Command(const string cmd_line) : m_cmd(cmd_line) {}

    virtual ~Command() {}

    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();

    virtual string GetLine() const;
    friend std::ostream& operator<<(std::ostream& os, const Command& cmd);
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
    string m_org_word;
public:
    ExternalCommand(const char *cmd_line, string first_word);

    virtual ~ExternalCommand() {}
    virtual string GetLine() const override;
    void execute() override;
};

class PipeCommand : public Command {
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {}

    void execute() override;
};

class WatchCommand : public Command {
    int m_interval;
    static Command* m_command;
    static bool m_isBg;
public:
    WatchCommand(const char *cmd_line);

    virtual ~WatchCommand() {}

    void execute() override;

    static void signalHandler(int sig_num);
};


class RedirectionCommand : public Command {
    unsigned int m_std_fd;
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    static std :: string m_lastPwd;
public:

    ChangeDirCommand(const char *cmd_line);
    //ChangeDirCommand(const char *cmd_line, std :: string lastPwd);

    virtual ~ChangeDirCommand() {}
    friend bool checkValid(const char* line);
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmdLine);

    virtual ~GetCurrDirCommand() {}

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class ChangePrompt : public BuiltInCommand {
    string m_prompt;
public:
    ChangePrompt(const char *cmd_line);

    virtual ~ChangePrompt() {}

    void execute() override;
};

class AuxAliasommand: public Command {
    public:
        AuxAliasommand(const char *cmd_line);
        virtual ~AuxAliasommand() {}
        void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
private:
    JobsList* m_jobs;
    bool m_2kill;
public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {}

    void execute() override;
};


class JobsList {
public:
    class JobEntry {
    private:
        unsigned int m_id;
        bool m_is_finished;
        Command* m_cmd;
        pid_t m_pid;
    public:
        JobEntry(bool is_stopped, unsigned int id,Command* cmd, pid_t pid);
        friend std::ostream& operator<<(std::ostream& os, const JobEntry& job);
        Command* GetCommand() const;
        pid_t Getpid() const;
        bool isFinished() const;
        void Done();
    };
    std::map<unsigned int, JobEntry> m_jobs;
    std ::set<unsigned int> m_max_ids;
public:
    JobsList();

    ~JobsList(){}

    void addJob(Command *cmd, pid_t pid, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);
    bool isEmpty() const;
};

class JobsCommand : public BuiltInCommand {
    JobsList* m_jobs;
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {}

    void execute() override;
};

class KillCommand : public BuiltInCommand {
private:
    unsigned int m_signum;
    int m_jobId;
    JobsList* m_jobs;
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
private:
    JobsList* m_jobs;
    int m_job_id;
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {}

    void execute() override;
};

class ListDirCommand : public BuiltInCommand {
    string m_path;
public:
    ListDirCommand(const char *cmd_line);

    virtual ~ListDirCommand() {}

    void execute() override;
};

class GetUserCommand : public BuiltInCommand {
    pid_t m_targetPid;
public:
    GetUserCommand(const char *cmd_line);

    virtual ~GetUserCommand() {}

    void execute() override;
};

class aliasCommand_DS{
    std :: vector<pair<std::string,std::string>> m_alias;
    public:
    void add_alias_command(std :: string name, std::string command);
    void remove_alias_command(std :: string name);
    void print_alias_command();
    bool checkInAlias(std::string alias);
    std::string TranslateAlias(std::string alias);
};


class aliasCommand : public BuiltInCommand {
    aliasCommand_DS *m_aliasDS;
public:
    aliasCommand(const char *cmd_line, aliasCommand_DS *aliasDS);
    virtual ~aliasCommand() {}
    void execute() override;
};

class unaliasCommand : public BuiltInCommand {
    aliasCommand_DS *m_aliasDS;
public:
    unaliasCommand(const char *cmd_line ,aliasCommand_DS *aliasDS);

    virtual ~unaliasCommand() {}

    void execute() override;
};


class SmallShell {
private:
    std::string m_prompt;
    aliasCommand_DS m_aliasDS;
    JobsList m_jobsList;
    pid_t m_fgPid;
    SmallShell();

public:
    Command *CreateCommand(const char *cmd_line);
    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();
    void executeCommand(const char *cmd_line);
    std::string GetPrompt();
    void SetPrompt(const std::string& prompt);
    void addJob(Command* cmd, pid_t pid);
    void setWorkingPid(pid_t pid);
    pid_t getWorkingPid() const;
    bool isWaiting() const;
};

#endif //SMASH_COMMAND_H_
