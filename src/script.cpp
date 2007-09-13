#include <unistd.h>
#include <limits.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "script.h"

/* hard coded paths */
#define VLOCK_SCRIPT_DIR PREFIX "/lib/vlock/scripts"

static void get_dependency(const char *path, const char *name, list<string> dependency);
static pid_t launch_script(const char *path, int pipe_fd);
static void ensure_death(pid_t pid);

Script::Script(string name) : Plugin(name)
{
  char *path;
  int pipe_fds[2];

  /* format the plugin path */
  if (asprintf(&path, "%s/%s", VLOCK_SCRIPT_DIR, name.c_str()) < 0)
    throw std::bad_alloc();

  /* load dependencies */
  for (vector<string>::iterator it = dependency_names.begin();
      it != dependency_names.end(); it++)
      get_dependency(path, (*it).c_str(), dependencies[*it]);

  if (pipe(pipe_fds) < 0)
    throw PluginException("pipe() failed");

  fd = pipe_fds[1];

  pid = launch_script(path, pipe_fds[0]);

  free(path);

  throw PluginException("No way!");
}

Script::~Script()
{
  (void) close(fd);
  ensure_death(pid);
}

bool Script::call_hook(string name)
{
  (void) name;
  return true;
}

bool close_all_fds(void)
{
  rlimit r;

  if (getrlimit(RLIMIT_NOFILE, &r) < 0)
    return false;

  // close all file descriptors except stdio
  for (unsigned int i = 0; i < r.rlim_cur; i++) {
    switch (i) {
      case STDIN_FILENO:
      case STDOUT_FILENO:
      case STDERR_FILENO:
        break;
      default:
        close(i);
    }
  }

  return true;
}

static void get_dependency(const char *path, const char *name, list<string> dependency)
{
  int pipe_fds[2];
  pid_t pid;
  timeval timeout;
  string data;
  string errmsg;

  if (pipe(pipe_fds) < 0)
    throw PluginException("pipe() failed");

  pid = fork();

  if (pid == 0) {
    // child
    int nullfd = open("/dev/null", O_RDWR);

    if (nullfd < 0)
      _exit(1);

    // redirect stdio
    (void) dup2(nullfd, STDIN_FILENO);
    (void) dup2(pipe_fds[1], STDOUT_FILENO);
    (void) dup2(nullfd, STDERR_FILENO);

    // close all other file descriptors
    if (!close_all_fds())
      _exit(1);

    // drop privileges
    setgid(getgid());
    setuid(getuid());

    (void) execl(path, path, name, NULL);

    _exit(1);
  }

  (void) close(pipe_fds[1]);

  if (pid < 0) {
    (void) close(pipe_fds[0]);
    throw PluginException("fork() failed");
  }

  for (;;) {
    char buffer[LINE_MAX];
    ssize_t len;

    fd_set read_fds;

    FD_ZERO(&read_fds);
    FD_SET(pipe_fds[0], &read_fds);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (select(pipe_fds[0]+1, &read_fds, NULL, NULL, &timeout) != 1) {
      errmsg = errmsg + "timeout while reading dependency '" + name + "' from '" + path + "'";
      goto out;
    }

    len = read(pipe_fds[0], buffer, sizeof buffer - 1);

    if (len <= 0)
      break;

    buffer[len] = '\0';
    data += buffer;

    if (data.length() > LINE_MAX) {
      errmsg = errmsg + "too much data while reading dependency '" + name + "' from '" + path + "'";
      goto out;
    }
  }

  for (size_t pos1 = 0, pos2 = data.find('\n', pos1);
      pos1 < data.length();
      pos1 = pos2+1, pos2 = data.find('\n', pos1))
    if ((pos2 - pos1) > 1)
      dependency.push_back(data.substr(pos1, pos2 - pos1));

out:
  (void) close(pipe_fds[0]);
  ensure_death(pid);

  if (errmsg.length() == 0)
    throw PluginException(errmsg);
}

static pid_t launch_script(const char *path, int pipe_fd)
{
  pid_t pid = fork();

  if (pid == 0) {
    // child
    int nullfd = open("/dev/null", O_RDWR);

    if (nullfd < 0)
      _exit(1);

    // redirect stdio
    (void) dup2(pipe_fd, STDIN_FILENO);
    (void) dup2(nullfd, STDOUT_FILENO);
    (void) dup2(nullfd, STDERR_FILENO);

    // close all other file descriptors
    if (!close_all_fds())
      _exit(1);

    // drop privileges
    setgid(getgid());
    setuid(getuid());

    (void) execl(path, path, "hooks", NULL);

    _exit(1);
  }

  (void) close(pipe_fd);

  if (pid > 0)
    return pid;
  else
    throw PluginException("fork() failed");
}

static void ensure_death(pid_t pid)
{
  int status;

  if (waitpid(pid, &status, WNOHANG) == pid)
    return;

  (void) kill(pid, SIGTERM);

  if (waitpid(pid, &status, WNOHANG) == pid)
    return;

  (void) kill(pid, SIGKILL);

  (void) waitpid(pid, &status, WNOHANG);
}
