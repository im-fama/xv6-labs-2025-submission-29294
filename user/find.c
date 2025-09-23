#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

char* fmtname(char *path) {
  static char buf[DIRSIZ+1];
  char *p;
  for(p = path + strlen(path); p >= path && *p != '/'; p--) ;
  p++;
  if(strlen(p) >= DIRSIZ) return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)] = 0;
  return buf;
}

int do_exec = 0;
char *cmdargs[MAXARG];
int cmdargc = 0;

void runexec(char *file) {
  if(!do_exec) return;

  char *argv[MAXARG];
  for(int i=0; i<cmdargc && i<MAXARG-2; i++)
    argv[i] = cmdargs[i];
  argv[cmdargc] = file;
  argv[cmdargc+1] = 0;

  if(fork() == 0){
    exec(argv[0], argv);
    fprintf(2, "exec %s failed\n", argv[0]);
    exit(1);
  }
  wait(0);
}

void find(char *path, char *name) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0)
    return;
  if(fstat(fd, &st) < 0){
    close(fd);
    return;
  }

  if(st.type == T_FILE){
    if(strcmp(fmtname(path), name) == 0){
      if(do_exec) runexec(path);
      else printf("%s\n", path);
    }
  } else if(st.type == T_DIR){
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
      close(fd);
      return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0) continue;
      if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf, name);
    }
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if(argc < 3){
    fprintf(2, "Usage: find <path> <name> [-exec cmd [args...]]\n");
    exit(1);
  }

  if(argc > 3 && strcmp(argv[3], "-exec") == 0){
    do_exec = 1;
    cmdargc = argc - 4;
    for(int i=0; i<cmdargc; i++)
      cmdargs[i] = argv[4 + i];
  }

  find(argv[1], argv[2]);
  exit(0);
}

