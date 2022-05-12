
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <utime.h>
#include <time.h>
#include "fcntl.h"
#include <list>
// #include <utime.h>


using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}
string _ltrimr(const std::string& s)
{
    size_t start = s.find_first_not_of("\r\n");
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrimr(const std::string& s)
{
    size_t end = s.find_last_not_of("\r\n");
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trimr(const std::string& s)
{
    return _rtrimr(_ltrimr(s));
}
int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)));
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned long idx = str.find_last_not_of(WHITESPACE);
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

SmallShell::SmallShell():jobsList() {
this->jobsList.List->resize(MAX_NUM_OF_JOBS + 1);
this->timeOut= std::list<AlarmNote *>();/////
this->prompt="smash";
this->last_working_directory="";
this->pid=getpid();
this->fg_pid=EMPTY_FG;
this->curr_cmd= nullptr;
}

SmallShell::~SmallShell() =default;

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    string cmd_s = _trim(std::string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (string(cmd_line).find('>') != string::npos) {
        return new RedirectionCommand(cmd_line);
    }
    else if (string(cmd_line).find('|') != string::npos) {
        return new PipeCommand(cmd_line);
    }
    else if (firstWord == "chprompt") {
        return new ChangePrompt(cmd_line);
    }
    else if (firstWord == "pwd") {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord == "showpid") {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord == "cd") {
        return new ChangeDirCommand(cmd_line);
    }
    else if (firstWord=="jobs") {
        return new JobsCommand(cmd_line);
    }
    else if (firstWord=="kill") {
        return new KillCommand(cmd_line);
    }
    else if (firstWord=="fg") {
        return new ForegroundCommand(cmd_line);
    }
    else if (firstWord=="bg") {
        return new BackgroundCommand(cmd_line);
    }
    else if (firstWord=="quit") {
        return new QuitCommand(cmd_line);
    }
    else if (firstWord=="tail") {
        return new TailCommand(cmd_line);
    }
    else if (firstWord=="touch") {
        return new TouchCommand(cmd_line);
    }
    else if (firstWord=="timeout") {
        return new TimeOut(cmd_line);
    }
    else {
        return new ExternalCommand(cmd_line);
    }
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    SmallShell& smash = SmallShell::getInstance();
    smash.curr_cmd=cmd;
    cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

Command::Command(const char* cmd_line) : cmd_line(cmd_line) {
    char* args_array[COMMAND_ARGS_MAX_LENGTH];
    this->arguments=new char* [COMMAND_MAX_ARGS];
    num_of_args = _parseCommandLine(cmd_line, args_array);
    for (int i = 0; i < num_of_args; ++i) {
        arguments[i] = args_array[i];
    }
}

Command::~Command() {
    for (int i = 0; i < num_of_args; ++i) {
        delete[] arguments[i];
    }
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

ChangePrompt::ChangePrompt(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChangePrompt::execute() {
    SmallShell& smash = SmallShell::getInstance();
    if(this->arguments[1]== nullptr)
    {
        smash.prompt="smash";
    }
    else
    {
        smash.prompt=this->arguments[1];
    }
}
ChangeDirCommand::ChangeDirCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void ChangeDirCommand::execute()
{
    SmallShell& smash = SmallShell::getInstance();
    if((this->num_of_args==3&&strcmp(this->arguments[2],"&")!=0)||(this->num_of_args>=4)||(this->num_of_args==1))
    {
        std::cerr<<"smash error: cd: too many arguments"<<endl;
        return;
    }
    char* new_prev_dir = get_current_dir_name();
    if(strcmp(this->arguments[1],"-")==0)
    {
        if(smash.last_working_directory.empty())
        {
            std::cerr<<"smash error: cd: OLDPWD not set"<<endl;
            return;
        }
        else
        {
            newdir=smash.last_working_directory;
        }
    }
    else
    {
        newdir=this->arguments[1];
    }
    if(chdir(newdir.c_str())==-1)
    {
        perror("smash error: chdir failed");
        return;
    }
    smash.last_working_directory = new_prev_dir;
}

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash pid is " << smash.pid << endl;
}

void JobsList::printJobsList() {
    JobEntry* curr_job;
    removeFinishedJobs();
    for(int i=1;i<=MAX_NUM_OF_JOBS;i++)
    {
        curr_job=this->List->at(i);
        if(curr_job== nullptr)
        {
            continue;
        }
        std::cout<<"["<<curr_job->job_id<<"] "<< curr_job->discript <<" : "<<curr_job->job_pid<<" ";
        if(curr_job->job_status==bg)
        {
            std::cout<< difftime(time(nullptr), curr_job->insert_time) << " secs"<<endl;
        }
        else
        {
            std::cout<< difftime(time(nullptr), curr_job->insert_time) << " secs (stopped)"<<endl;
        }

    }
}

int JobsList::getLastJob() {
    int max=0;
    for(int i=1;i<=MAX_NUM_OF_JOBS;i++) {
        JobEntry *curr_job = this->List->at(i);
        if(curr_job!= nullptr)
        {
            max=i;
        }
    }
    return max;
}

int JobsList::getLastStoppedJob() {
    int max=0;
    for(int i=1;i<=MAX_NUM_OF_JOBS;i++) {
        JobEntry *curr_job = this->List->at(i);
        if(curr_job!= nullptr&&curr_job->job_status==stopped)
        {
            max=i;
        }
    }
    return max;
}

void JobsList::JobEntry::StopJob() {
    this->job_status=stopped;
    this->stopped_time=time(nullptr);

}

void JobsList::JobEntry::ContinueJob() {
    this->job_status=bg;
}

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char* dir = get_current_dir_name();
    if (dir != nullptr) {
        cout << dir << endl;
        delete dir;
    }
    else {
        perror("smash error: get_current_dir_name failed");
    }
}

QuitCommand::QuitCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void QuitCommand::execute() {
    std::vector<JobsList::JobEntry*>* list = SmallShell::getInstance().jobsList.List;
    JobsList::JobEntry* current_job;
    SmallShell::getInstance().jobsList.removeFinishedJobs();
    int counter = 0;
    if (this->num_of_args==2&&strcmp(this->arguments[1],"kill") == 0) {
        for (int i = 1; i <= MAX_NUM_OF_JOBS; i++) {
            current_job = list->at(i);
            if (current_job) {
                counter++;
            }
        }
        cout << "smash: sending SIGKILL signal to " << counter << " jobs:" << endl;
        SmallShell::getInstance().jobsList.killAllJobs();
    }
    exit(0);
}

void JobsList::killAllJobs() {
    std::vector<JobsList::JobEntry *> *list = SmallShell::getInstance().jobsList.List;
    JobsList::JobEntry* current_job;
    for (int i = 1; i <= MAX_NUM_OF_JOBS; i++) {
        current_job = list->at(i);
        if (current_job) {
            cout << current_job->job_pid << ": " << current_job->discript << endl;
            kill(current_job->job_pid, SIGKILL);
        }
    }
}

void JobsList::addJob(const char * cmd,int pid, JobStatus isStopped) {
    removeFinishedJobs();
    std::vector<JobsList::JobEntry *> *list = SmallShell::getInstance().jobsList.List;
    int max_job_id = getLastJob();
    int job_id = (max_job_id + 1);
    list->at(job_id) = new JobEntry();
    list->at(job_id)->job_pid = pid;
    list->at(job_id)->job_id = job_id;

    string help(cmd);
    list->at(job_id)->discript = new char[help.length()];
    strcpy( list->at(job_id)->discript, cmd);
    list->at(job_id)->job_status=isStopped;
    list->at(job_id)->insert_time = time(nullptr); //add stop_time
}

void JobsList::removeFinishedJobs() {
    std::vector<JobsList::JobEntry *> *list = SmallShell::getInstance().jobsList.List;
    JobEntry* current_job;
    for (int i = 1 ; i <= MAX_NUM_OF_JOBS; i++) {
        current_job = list->at(i);
        if(current_job== nullptr)
        {
            continue;
        }
        int wait_res=waitpid(current_job->job_pid, nullptr, WNOHANG);
        if (wait_res== current_job->job_pid) {
            list->at(i)= nullptr;
        }
    }
}

int JobsList::getJobByPid(int pid) {
    std::vector<JobsList::JobEntry *> *list = SmallShell::getInstance().jobsList.List;
    JobEntry* current_job;
    for (int i = 1; i <= MAX_NUM_OF_JOBS; i++) {
        current_job = list->at(i);
        if ((current_job != nullptr) && (pid == current_job->job_pid)) {
            return i;
        }
    }
    return 0;
}

ForegroundCommand::ForegroundCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

void ForegroundCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    JobsList::JobEntry *job;
    smash.jobsList.removeFinishedJobs();
    int job_id;
    if (this->num_of_args==1) {
        job_id = smash.jobsList.getLastJob();
    } else {
        try {
            if(this->num_of_args>2)
            {
                throw std::invalid_argument("");
            }
            job_id = stoi(this->arguments[1]);

        }catch (std::invalid_argument& ia)
        {
            std::cerr<<"smash error: fg: invalid arguments"<<endl;
            return;
        }
    }
    if(job_id<0)
    {
        std::cerr<<"smash error: fg: job-id "<<job_id<<" does not exist"<<endl;
        return;
    }
    if(job_id==0)
    {
        std::cerr<<"smash error: fg: jobs list is empty"<<endl;
        return;
    }

    job = smash.jobsList.List->at(job_id);
    if(job== nullptr)
    {
        std::cerr<<"smash error: fg: job-id "<<job_id<<" does not exist"<<endl;
        return;
    }

    smash.fg_pid=job->job_pid;
    if (job->job_status == stopped) {
        kill(job->job_pid, SIGCONT);
    }
    cout << job->discript << " : " << job->job_pid << endl;

    if(waitpid(job->job_pid, nullptr, WUNTRACED)==-1)
    {
        perror("smash error: waitpid failed");
        return;
    }

}

BackgroundCommand::BackgroundCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

void BackgroundCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    smash.jobsList.removeFinishedJobs();
    JobsList::JobEntry *job;
    int job_id;
    if (this->num_of_args==1) {
        job_id = smash.jobsList.getLastStoppedJob();
    } else {
        try {
            if (this->num_of_args > 2) {
                throw std::invalid_argument("");
            }
            job_id = stoi(this->arguments[1]);
            if(job_id<0||job_id>100)
            {
                std::cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
                return;
            }

        } catch (std::invalid_argument &ia) {
            std::cerr << "smash error: bg: invalid arguments" << endl;
            return;
        }
    }
    if (job_id == 0) {
        std::cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
        return;
    }

    job = smash.jobsList.List->at(job_id);
    if (job == nullptr) {
        std::cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
        return;
    }
    if (job->job_status == bg) {
        std::cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
        return;
    }
    if (job->job_status == stopped) {
        kill(job->job_pid, SIGCONT);
    }
    job->job_status = bg;
    cout << job->discript << " : " << job->job_pid << endl;
}
JobsCommand::JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void JobsCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    smash.jobsList.printJobsList();
}

KillCommand::KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void KillCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    smash.jobsList.removeFinishedJobs();
    JobsList::JobEntry *job;
    int job_id,sig_num;
    try {
        if (this->num_of_args != 3) {
            throw std::invalid_argument("");
        }

        job_id = stoi(this->arguments[2]);
        sig_num = stoi(this->arguments[1]);
        if(sig_num>=0||sig_num<=-63)
        {
            throw std::invalid_argument("");
        }
    } catch (std::invalid_argument &ia) {
        std::cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    if (job_id >100||job_id<=0) {
        std::cerr << "smash error: kill: job-id "<<job_id<<" does not exist"<<endl;
        return;
    }
    job = smash.jobsList.List->at(job_id);
    if (job == nullptr) {
        std::cerr << "smash error: kill: job-id "<<job_id<<" does not exist"<<endl;
        return;
    }
    kill(job->job_pid, -sig_num);
    std::cout << "signal number "<< -sig_num<<" was sent to pid "<<job->job_pid <<endl;
}

ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    pid_t p = fork();

    if (p == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (p == 0) {
        //son process
        if(setpgrp()==-1)
        {
            perror("smash error: setpgrp failed");
            return;
        }
        char cmd_modified_line[COMMAND_ARGS_MAX_LENGTH];
        strcpy(cmd_modified_line,cmd_line);
        strcpy(cmd_modified_line,_trim(cmd_line).c_str());
        if (_isBackgroundComamnd(cmd_line)) {
            _removeBackgroundSign(cmd_modified_line);
        }
        strcpy(cmd_modified_line,_trim(cmd_modified_line).c_str());
        char * full_array []= {(char*)"/bin/bash", (char*)"-c",(char*)cmd_modified_line, nullptr};
        if(execv("/bin/bash", full_array)== -1)
        {
            perror("smash error: execv failed");
            return;
        }
    }
    else
    {

        //god-father
    if (_isBackgroundComamnd(cmd_line)) {
        char cmd_modified_line[COMMAND_ARGS_MAX_LENGTH];
        strcpy(cmd_modified_line,cmd_line);
        strcpy(cmd_modified_line,_trimr(cmd_modified_line).c_str());
        smash.jobsList.addJob(cmd_modified_line, p, bg);
        //smash.jobsList.addJob(cmd_line, p, bg);
        smash.fg_pid=EMPTY_FG;
    }
    else {
        smash.fg_pid=p;
            if(waitpid(p, nullptr, WUNTRACED)==-1)
            {
                perror("smash error: waitpid failed");
                return;
            }
            //waitpid(p,nullptr,0);
        smash.fg_pid = EMPTY_FG;
        }
    }
}

RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line) {}

void RedirectionCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    string cmd_line_s= string(cmd_line);
    bool is_append=cmd_line_s.find(">>") != string::npos;
    int first_part=(int)cmd_line_s.find('>');
    char cmd_modified_line[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_modified_line,cmd_line_s.substr(0,first_part).c_str());
    Command *cmd=smash.CreateCommand(cmd_modified_line);
    string txt_file=_trim(cmd_line_s.substr(first_part));
    if(txt_file[0]=='>')
    {
        txt_file=txt_file.substr(1,txt_file.length());
    }
    if(txt_file[0]=='>')
    {
        txt_file=txt_file.substr(1,txt_file.length());
    }
    txt_file=_trim(txt_file);
    int output_channel= dup(1);
    int fd=open(txt_file.c_str(),O_CREAT|O_WRONLY|(is_append ? O_APPEND:O_TRUNC),0655);
    if(fd==-1)
    {
        perror("smash error: open failed");
        return;
    }
    if(dup2 (fd,1)==-1)
    {
        perror("smash error: dup2 failed");
        return;
    }
    if(close(fd)==-1)
    {
        perror("smash error: close failed");
        return;
    }
    cmd->execute();

    if(dup2(output_channel,1) == -1)
    {
        perror("smash error: dup2 failed");
        return;
    }
    if(close(output_channel) == -1) {
        perror("smash error: close failed");
        return;
    }
}

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {}

void PipeCommand::execute() {
    Command *cmd1;
    Command *cmd2;
    SmallShell &smash = SmallShell::getInstance();
    string cmd_line_s= _trim(string(cmd_line));

    if(cmd_line_s.find("|&") != string::npos)
    {
        int first_part=(int)cmd_line_s.find("|&");
        char cmd_modified_line1[COMMAND_ARGS_MAX_LENGTH];
        char cmd_modified_line2[COMMAND_ARGS_MAX_LENGTH];
        strcpy(cmd_modified_line1,cmd_line_s.substr(0,first_part).c_str());
        cmd1=smash.CreateCommand(cmd_modified_line1);
        strcpy(cmd_modified_line2,cmd_line_s.substr(first_part+2,cmd_line_s.length()-first_part).c_str());
        cmd2=smash.CreateCommand(cmd_modified_line2);
        int output_channel= dup(2);//duplicate stderr
        if(output_channel==-1)
        {
            perror("smash error: dup failed");
            return;
        }
        int input_channel= dup(0);//duplicate stdin
        if(input_channel==-1)
        {
            perror("smash error: dup failed");
            return;
        }
        if(dup2 (output_channel,0)==-1)//
        {
            perror("smash error: dup2 failed");
            return;
        }
        if(close (input_channel)==-1)
        {
            perror("smash error: close failed");
            return;
        }
        if(close (output_channel)==-1)
        {
            perror("smash error: close failed");
            return;
        }
    }
    else
    {
        int first_part=(int)cmd_line_s.find('|');
        char cmd_modified_line1[COMMAND_ARGS_MAX_LENGTH];
        char cmd_modified_line2[COMMAND_ARGS_MAX_LENGTH];
        strcpy(cmd_modified_line1, _trimr(cmd_line_s.substr(0,first_part)).c_str());
        cmd1=smash.CreateCommand(cmd_modified_line1);
        strcpy(cmd_modified_line2,_trimr(cmd_line_s.substr(first_part+1,cmd_line_s.length()-first_part)).c_str());
        cmd2=smash.CreateCommand(cmd_modified_line2);
        int output_channel= dup(1);//duplicate stdout
        if(output_channel==-1)
        {
            perror("smash error: dup failed");
            return;
        }
        int input_channel= dup(0);
        if(input_channel==-1)
        {
            perror("smash error: dup failed");
            return;
        }
        if(dup2 (output_channel,1)==-1)
        {
            perror("smash error: dup2 failed");
            return;
        }
        cmd1->execute();
        cmd2->execute();
        if(close (input_channel)==-1)
        {
            perror("smash error: close failed");
            return;
        }
        if(close (output_channel)==-1)
        {
            perror("smash error: close failed");
            return;
        }
    }

}

TailCommand::TailCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void TailCommand::execute() {
    string text_file;
    int lines_number;
    try{
        if(num_of_args == 3) {
            lines_number = (-1)* stoi(this->arguments[1]);
            if(lines_number < 0)
            {
                throw std::invalid_argument("");
            }
            text_file=this->arguments[2];
        }
        else if(num_of_args == 2)
        {
            lines_number = 10;
            text_file = this->arguments[1];
        }
        else
        {
            throw std::invalid_argument("");
        }
    }
    catch (std::invalid_argument& ia)
    {
        std::cerr<<"smash error: tail: invalid arguments"<<endl;
        return;
    }

    int fd=open(text_file.c_str(), O_RDONLY);
    if(fd == -1)
    {
        perror("smash error: open failed");
        return;
    }
    off_t bgn_pos = lseek(fd, 0, SEEK_CUR);
    if(bgn_pos == -1) 
    {
        perror("smash error: lseek failed");
        return;
    }
    off_t end_pos = lseek(fd, 0, SEEK_END);
    if(end_pos == -1) 
    {
        perror("smash error: lseek failed");
        return;
    }
    if (bgn_pos == end_pos) //the current file is empty
    {
        return;
    }
    char * buffer = new char[1];
    int count_of_chars = 0, count_of_lines = 0;
    while(count_of_lines != lines_number)
    {
        int dist_from_bgn_of_file = lseek(fd, --count_of_chars, SEEK_END);
        if (dist_from_bgn_of_file == -1)
        {
            perror("smash error: lseek failed");
            return;
        }
        else if (dist_from_bgn_of_file > 0)
        {
            if(read(fd,buffer, 1) == -1)
            {
                perror("smash error: read failed");
                return;
            }
            if(*buffer == '\n')
            {
                count_of_lines += (count_of_chars == -1) ? 0 : 1;
            }
        }
        else //reached beginning of file
        {
            break;
        }
    }
    long read1;
    while (true)
    {
       read1 = read(fd, buffer, 1);
        if(read1 == 0)
        {
            break;
        }
        if(read1 == -1)
        {
            perror("smash error: read failed");
            return;
        }
        if(write(1, buffer, 1) != 1)
        {
            perror("smash error: write failed");
            return;
        }
    }
    if(close(fd) == -1)
    {
        perror("smash error: close failed");
        return;
    }
}

TouchCommand::TouchCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void TouchCommand::execute() {
    if(this->num_of_args!=3)
    {
        cerr << "smash error: touch: invalid arguments" << endl;
        return;
    }
    int secs,mins,hours,days,months,years;
    std::stringstream ss(this->arguments[2]);
    char seperator = ':';
    ss>>secs>>seperator>>mins>>seperator>>hours>>seperator>>days>>seperator>>months>>seperator>>years;
    struct tm tm1;
    tm1.tm_sec = secs;
    tm1.tm_min = mins;
    tm1.tm_hour = hours;
    tm1.tm_mday = days;
    tm1.tm_mon = months - 1;
    tm1.tm_year = years - 1900;
    tm1.tm_isdst=-1;
    tm1.tm_yday=-1;
    tm1.tm_wday=-1;

    time_t tm_ob = mktime(&tm1);
    struct utimbuf buf;
    buf.actime = tm_ob;
    buf.modtime = tm_ob;
    if(utime(this->arguments[1],&buf)==-1)
    {
        perror("smash error: utime failed");
        return;
    }
}

TimeOut::TimeOut(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void TimeOut::execute() {
    int timeout, i = 2;
    string cmd_line_s = string(cmd_line);
    SmallShell& smash = SmallShell::getInstance();
try
{
    if(num_of_args<3) {
        throw std::invalid_argument("");
    }
    timeout = stoi(arguments[1]);
    if(timeout <= 0)
    {
        throw std::invalid_argument("");
    }

}catch (std::invalid_argument& ia)
{
    std::cerr<<"smash error: timeout: invalid arguments"<<endl;
    return;
}
    char cmd_modified_line[COMMAND_ARGS_MAX_LENGTH];
for(char & i : cmd_modified_line)
{
    i='\0';
}
    while(i < num_of_args) {
        strcat(cmd_modified_line, std::string(arguments[i]).c_str());
        strcat(cmd_modified_line, " ");
        i++;
    }
    pid_t p = fork();

    if (p == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (p == 0) {
        //son process
        if(setpgrp()==-1)
        {
            perror("smash error: setpgrp failed");
            return;
        }
        if (_isBackgroundComamnd(cmd_line)) {
            _removeBackgroundSign(cmd_modified_line);
        }
        strcpy(cmd_modified_line,_trim(cmd_modified_line).c_str());
        char * full_array []= {(char*)"/bin/bash", (char*)"-c",(char*)cmd_modified_line, nullptr};
        if(execv("/bin/bash", full_array)== -1)
        {
            perror("smash error: execv failed");
            return;
        }
    }
    else
    {
        smash.timeOut.push_back(new AlarmNote(p,timeout, _trimr(cmd_line).c_str()));
        int lowest_alarm = timeout;
        for (auto it = smash.timeOut.begin(); it != smash.timeOut.end(); ++it) 
        {
            if((*it)->duration - difftime(std::time(nullptr), (*it)->insert_time) < lowest_alarm)
            {
                lowest_alarm = (*it)->duration - difftime(std::time(nullptr), (*it)->insert_time);
            }
        }
        alarm(lowest_alarm);
        if (_isBackgroundComamnd(cmd_line)) {
            strcpy(cmd_modified_line,cmd_line);
            strcpy(cmd_modified_line,_trimr(cmd_modified_line).c_str());
            smash.jobsList.addJob(cmd_modified_line, p, bg);
            smash.fg_pid=EMPTY_FG;
        }
        else {
            smash.fg_pid = p;
            if(waitpid(p, nullptr, WUNTRACED) == -1)
            {
                perror("smash error: waitpid failed");
                return;
            }
            smash.fg_pid = EMPTY_FG;
        }
    }
}

AlarmNote::AlarmNote( int pid,int duration,const char* discript):pid(pid),duration(duration),insert_time(time(nullptr))
{
    std::string help(discript);
    this->discript = new char[help.length()];
    strcpy( this->discript, discript);
}
