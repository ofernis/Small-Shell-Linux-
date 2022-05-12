#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <string.h>
#include <sys/wait.h>

using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";
string _ltrim1(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim1(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim1(const std::string& s)
{
    return _rtrim1(_ltrim1(s));
}

void ctrlZHandler(int sig_num) {
	std::cout << "smash: got ctrl-Z" << endl;
  SmallShell& smash = SmallShell::getInstance();
  if (smash.fg_pid == EMPTY_FG) {
    return;
  }
  int job_id=smash.jobsList.getJobByPid(smash.fg_pid);
  if(job_id==0)
  {
      char cmd_modified_line[COMMAND_ARGS_MAX_LENGTH];
      strcpy(cmd_modified_line,smash.curr_cmd->cmd_line);
      strcpy(cmd_modified_line,_trim1(cmd_modified_line).c_str());
      smash.jobsList.addJob(cmd_modified_line, smash.fg_pid, stopped);
  }
    job_id=smash.jobsList.getJobByPid(smash.fg_pid);
    smash.jobsList.List->at(job_id)->StopJob();
  if (kill(smash.fg_pid,SIGSTOP) == -1) {
    perror("smash error: kill failed");
    return;
  }
  std::cout << "smash: process " << smash.fg_pid << " was stopped" << endl;
  smash.fg_pid = EMPTY_FG;
}

void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << endl;
  SmallShell& smash = SmallShell::getInstance();
  if (smash.fg_pid == EMPTY_FG) {
    return;
  }
  if (kill(smash.fg_pid,SIGKILL) == -1) {
    perror("smash error: kill failed");
    return;
  }
  cout << "smash: process " << smash.fg_pid << " was killed" << endl;
    int job_id=smash.jobsList.getJobByPid(smash.fg_pid);
  if(job_id!= 0)
  {
      smash.jobsList.List->at(job_id)= nullptr;
  }
  smash.fg_pid = EMPTY_FG;
}

void alarmHandler(int sig_num) {
  cout << "smash: got an alarm" << endl;
    SmallShell& smash = SmallShell::getInstance();
    for (auto it = smash.timeOut.begin(); it != smash.timeOut.end(); it++)
    {
      if(difftime(time(nullptr), (*it)->insert_time) >= (*it)->duration)
      {
          int job_id = smash.jobsList.getJobByPid((*it)->pid);
          int res = waitpid((*it)->pid, nullptr, WNOHANG);

          if((job_id != 0) && (smash.jobsList.List->at(job_id) != nullptr))
          {
            smash.jobsList.List->at(job_id) = nullptr;
          }
          if(res <= 0)
          {
              if(kill((*it)->pid, SIGKILL) == 0)
              {
                  cout << "smash: " << (*it)->discript << " timed out!" << endl;
              }
          }
          smash.timeOut.erase(it);
          it = smash.timeOut.begin();
      }
  }
  int lowest_alarm = MAX_INT;
  for (auto it = smash.timeOut.begin(); it != smash.timeOut.end(); it++) 
  {
      if((*it)->duration - difftime(time(nullptr), (*it)->insert_time) < lowest_alarm)
      {
        lowest_alarm = (*it)->duration - difftime(time(nullptr), (*it)->insert_time);
      }
  }
  alarm(lowest_alarm);
}