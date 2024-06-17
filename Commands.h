#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_


#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#include <map>
#include <vector>
#include <string>
#include <set>

using namespace std;

vector<string> SMASH_COMMANDS = {"pwd", "cd", "chprompt", "showpid"};
class Command {
// TODO: Add your data members
private:
    const pid_t m_pid;
protected:
    const std::string m_cmd;
public:
    Command(const char *cmd_line) : m_cmd(cmd_line), m_pid(getpid()) {}

    virtual ~Command() {}

    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
    pid_t GetPid() const;
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line) : Command(cmd_line){}

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {}

    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {}

    void execute() override;
};

class WatchCommand : public Command {
    // TODO: Add your data members
public:
    WatchCommand(const char *cmd_line);

    virtual ~WatchCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members 
    static std :: string m_lastPwd;
public:

    ChangeDirCommand(const char *cmd_line);
    //ChangeDirCommand(const char *cmd_line, std :: string lastPwd);

    virtual ~ChangeDirCommand() {}
    friend bool checkValid(const char* line);
    void execute() override;
};
//TODO?
std :: string ChangeDirCommand :: m_lastPwd;

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
public:
    ChangePrompt(const char *cmd_line);

    virtual ~ChangePrompt() {}

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {}

    void execute() override;
};


class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
    private:
        unsigned int m_id;
        bool m_is_finished;
        const Command* m_cmd;
    public:
        JobEntry(bool is_stopped, unsigned int id,Command* cmd);
        friend std::ostream& operator<<(std::ostream& os, const JobEntry& job);
        const Command* GetCommand() const;
    };
    // TODO: Add your data members
    std::map<unsigned int, JobEntry> m_jobs;
    std ::set<unsigned int> m_max_ids;
    int m_last_job_id;
public:
    JobsList();

    ~JobsList(){}

    void addJob(Command *cmd, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {}

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {}

    void execute() override;
};

class ListDirCommand : public BuiltInCommand {
public:
    ListDirCommand(const char *cmd_line);

    virtual ~ListDirCommand() {}

    void execute() override;
};

class GetUserCommand : public BuiltInCommand {
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
    // TODO: Add your data members
    std::string m_prompt;
    aliasCommand_DS m_aliasDS;
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
    // TODO: add extra methods as needed
    std::string GetPrompt();
    void SetPrompt(const std::string& prompt);
};

#endif //SMASH_COMMAND_H_
