#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int myReadLine(int fd, char* line) {
  char ch;
  int offset = 0;
  while(read(fd,&ch,1)>0) {
    line[offset] = ch;
    if (ch == '\n') {
      line[offset] = '\0';
      return 1;
    }
    if (ch == EOF) {
      line[offset] = '\0';
      return 0;
    }
    offset += 1;
  }
  return -1;
}
int main(int argc, char *argv[]) {
  // prepare for trace system call
  char** cmdArgs = malloc(sizeof(char*)*(argc + 2));
  cmdArgs[0] = "strace";
  cmdArgs[1] = "-T";
  for (int i = 1; i <= argc; i++){
    cmdArgs[i+1] = argv[i];
  }
  char *pathvar = getenv("PATH"); 
  char* path = malloc(sizeof(char)*(strlen("PATH=") + strlen(pathvar)+1));
  memset(path, '\0', strlen("PATH=") + strlen(pathvar)+1);
  strcat(path, "PATH=");
  strcat(path, pathvar);
  char *exec_envp[] = { path, NULL, };
  char *test[] = { "strace", "-T", "ls", NULL, };
  // execute program

  int pipefds[2];
	if(pipe(pipefds) < 0){
		perror("pipe");
    assert(0);
	}
  pipe(pipefds);
  int pid = -1;
  pid = fork();
  if (pid == 0) {
    close(pipefds[0]);
    dup2(pipefds[1], fileno(stderr));
    int fd = open("/dev/null",O_RDWR);
    dup2(fd, fileno(stdout));
    // 子进程，执行strace命令
    execve("/usr/bin/strace", cmdArgs, exec_envp);
    assert(0);
    // 不应该执行此处代码，否则execve失败，出错处理
  } else {
    close(pipefds[1]);
    char buf[512];
    while(myReadLine(pipefds[0], buf) > 0) {
      int len = strlen(buf);
      int left = -1;
      int right = len-1;
      int leftparameter = -1;
      for (int i = len-1; i >= 0; i--) {
        if (buf[i] == '<') {
          left = i;
        }
      }
      for (int i = 0; i < len; i++) {
        if (buf[i] == '(') {
          leftparameter = i;
        }
      }
      if (buf[right]=='>' && leftparameter < 15){
        assert(leftparameter >= 0);
        printf("%d\n", leftparameter);
        char time[100];
        char syscall[50];
        memset(syscall, '\0', 50);
        memset(time, '\0', 100);
        memcpy(time, &buf[left+1], (right-left-1));
        memcpy(syscall, &buf[0], leftparameter);
        // printf("left: %c\tright: %c\n",buf[left] , buf[right]);
        printf("syscall: %s\ttime: %s\n", syscall, time);
      }
      memset(buf, '\0', sizeof(buf));
    }
    // 父进程，读取strace输出并统计
    // printf("BBB\n");
    // assert(0);
    return 0;
  }
  return 0;
  perror(argv[0]);
  exit(EXIT_FAILURE);
}
