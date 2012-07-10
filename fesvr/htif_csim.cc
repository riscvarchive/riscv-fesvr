#include "htif_csim.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

htif_csim_t::htif_csim_t(int ncores, const char* progname, std::vector<char*> args)
  : htif_t(ncores)
{
  int fromhost[2], tohost[2], flags;
  assert(pipe(fromhost) == 0);
  assert(pipe(tohost) == 0);

  assert((flags = fcntl(fromhost[0], F_GETFL)) >= 0);
  assert(fcntl(fromhost[0], F_SETFL, flags & ~O_NONBLOCK) == 0);

  assert((flags = fcntl(tohost[0], F_GETFL)) >= 0);
  assert(fcntl(tohost[0], F_SETFL, flags & ~O_NONBLOCK) == 0);

  pid = fork();
  assert(pid >= 0);

  if (pid == 0)
  {
    char fromhost_arg[32], tohost_arg[32];
    sprintf(fromhost_arg, "+fromhost=%d", fromhost[0]);
    sprintf(tohost_arg, "+tohost=%d", tohost[1]);

    args.insert(args.begin(), const_cast<char*>(progname));
    args.push_back(fromhost_arg);
    args.push_back(tohost_arg);

    char* args_array[args.size()+1];
    memcpy(args_array, &args[0], args.size() * sizeof(char*));
    args_array[args.size()] = 0;

    assert(execvp(args_array[0], args_array) == 0);
  }

  close(fromhost[0]);
  close(tohost[1]);
  
  fdin = tohost[0];
  fdout = fromhost[1];
}

htif_csim_t::~htif_csim_t()
{
  kill(pid, SIGTERM);
  for (int cnt=0; cnt<100; cnt++)
  {
    int ret;
    waitpid(pid, &ret, WNOHANG);
    if (WIFSIGNALED(ret))
      return;
    usleep(10*1000);
  }
  kill(pid, SIGKILL);
}
