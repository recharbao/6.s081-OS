#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}


void find(char* path, char* name)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    // printf("path = %s\n", path);

    if ((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
    return;
  }
            
    switch (st.type){
        case T_FILE:
        
            p = fmtname(path);
            
            while (*p != ' ' && *name != 0 && *p == *name){p++, name++;}
            // printf("p = %s\n", p);
            // printf("name = %d\n", *name);

            if (*name == 0 && *p == ' '){
                printf("%s\n", path);
            }
            break;
        
        case T_DIR:
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';

            int n = 2;
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if (n-- > 0 || de.inum == 0) continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(buf, name);
            }
            break;
    }

    close(fd);
    
}


int main(int argc, char* argv[])
{
    char* path = argv[1];
    char* name = argv[2];

    find(path, name);

    exit();
}