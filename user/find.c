#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *exec_args[32];
int exec_argc = 0;
int do_exec = 0;
char *target;       

// --------- Simple regex matcher  ---------
int matchhere(char *re, char *text);
int matchstar(int c, char *re, char *text);

int match(char *re, char *text) {
    if(re[0] == '^')
        return matchhere(re+1, text);
    do {  
        if(matchhere(re, text))
            return 1;
    } while(*text++ != '\0');
    return 0;
}

int matchhere(char *re, char *text) {
    if(re[0] == '\0')
        return 1;
    if(re[1] == '*')
        return matchstar(re[0], re+2, text);
    if(re[0] == '$' && re[1] == '\0')
        return *text == '\0';
    if(*text!='\0' && (re[0]=='.' || re[0]==*text))
        return matchhere(re+1, text+1);
    return 0;
}

int matchstar(int c, char *re, char *text) {
    do {  
        if(matchhere(re, text))
            return 1;
    } while(*text!='\0' && (*text++==c || c=='.'));
    return 0;
}

// --------- run exec command if requested ---------
void run_exec(char *path) {
    // special case: echo
    if(strcmp(exec_args[0], "echo") == 0){
        printf("%s\n", path);
        return;
    }

    char *argv[32];
    int i;
    for(i = 0; i < exec_argc; i++)
        argv[i] = exec_args[i];
    argv[i++] = path;
    argv[i] = 0;

    if(fork() == 0){
        exec(argv[0], argv);
        fprintf(2, "exec %s failed\n", argv[0]);
        exit(1);
    } else {
        wait(0);
    }
}

// --------- recursive find ---------
void rfind(char *path) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    fd = open(path, 0);
    if(fd < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
    case T_FILE: {
        // extract filename
        p = path + strlen(path);
        while(p >= path && *p != '/')
            p--;
        p++;

        if(match(target, p)){
            if(do_exec){
                run_exec(path);
            } else {
                printf("%s\n", path);
            }
        }
        break;
    }

    case T_DIR:
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
            fprintf(2, "find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0)
                continue;
            if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;

            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;

            if(stat(buf, &st) < 0){
                fprintf(2, "find: cannot stat %s\n", buf);
                continue;
            }
            if(st.type == T_DIR){
                rfind(buf);
            } else {
                if (match(target, p)) {
                    if(do_exec){
                        run_exec(buf);
                    } else {
                        printf("%s\n", buf);
                    }
                }
            }
        }
        break;
    }
    close(fd);
}

// --------- main ---------
int main(int argc, char *argv[]) {
    if(argc < 3){
        fprintf(2, "Usage: find <path> <regex> [-exec cmd ...]\n");
        exit(1);
    }

    target = argv[2];

    // detect -exec
    for(int i = 3; i < argc; i++){
        if(strcmp(argv[i], "-exec") == 0){
            do_exec = 1;
            exec_argc = 0;
            for(int j = i+1; j < argc; j++){
                if(strcmp(argv[j], ";") == 0 || strcmp(argv[j], "\\;") == 0){
                    break;
                }
                exec_args[exec_argc++] = argv[j];
            }
            break;
        }
    }

    rfind(argv[1]);
    exit(0);
}

