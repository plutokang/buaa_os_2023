#include <lib.h>
int flag[256];
int filecnt;
int dircnt;
void treedir(char *, int);
void treefile(char *, u_int, char *, int, int);
void tree(char *path)
{
    int r;
    struct Stat st;
    if ((r = stat(path, &st)) < 0)
    {
        printf("stat fail");
        exit();
    }
    if (!st.st_isdir)
    {
        printf("0 directories, 0 files\n");
        exit();
    }
    printf("%s\n", path);
    treedir(path, 0);
}
void treedir(char *path, int depth)
{
    int fdnum;
    struct Fd *fd;
    struct Filefd *ffd;
    int n;
    if ((fdnum = open(path, O_RDONLY)) < 0)
    {
        user_panic("open %s: %d", path, fdnum);
    }
    fd = (struct Fd *)num2fd(fdnum);
    ffd = (struct Filefd *)fd;
    int size = ffd->f_file.f_size;
    int va = (int)fd2data(fd);
    for (int i = 0; i < size; i += BY2FILE)
    {
        struct File *file;
        file = (struct File *)(va + i);
        if (file->f_name[0] == 0)
        {
            break;
        }
        int islast = 0;
        if (i == size || (file + 1)->f_name[0] == 0)
        {
            islast = 1;
        }
        treefile(path, file->f_type == FTYPE_DIR, file->f_name, depth + 1, islast);
    }
}
void treefile(char *path, u_int isdir, char *name, int step, int islast)
{
    char *sep;

    if (flag['d'] && !isdir)
    {
        return;
    }

    for (int i = 0; i < step - 1; i++)
    {
        printf("   ");
    }
    if (islast)
    {
        printf("## ");
    }
    else
    {
        printf("** ");
    }
    if (path[0] && path[strlen(path) - 1] != '/')
    {
        sep = "/";
    }
    else
    {
        sep = "";
    }
    if (flag['f'] && path)
    {
        if (isdir)
        {
            printf("\033[0;34m%s\033[0m", path);
            printf("\033[0;34m%s\033[0m", sep);
        }
        else
        {
            printf("%s%s", path, sep);
        }
    }
    if (isdir)
    {
        printf("\033[0;34m%s\033[0m\n", name);
    }
    else
    {
        printf("%s\n", name);
    }
    if (isdir)
    {
        dircnt += 1;
        char newpath[256];
        strcpy(newpath, path);
        int namelen = strlen(name);
        int pathlen = strlen(path);

        if (strlen(sep) != 0)
        {
            newpath[pathlen] = '/';
            for (int i = 0; i < namelen; i++)
            {
                newpath[pathlen + i + 1] = name[i];
            }
            newpath[pathlen + namelen + 1] = 0;
            treedir(newpath, step);
        }
        else
        {
            for (int i = 0; i < namelen; i++)
            {
                newpath[pathlen + i] = name[i];
            }
            newpath[pathlen + namelen] = 0;
            treedir(newpath, step);
        }
    }
    else
    {
        filecnt += 1;
    }
}
void usage(void)
{
    printf("usage: tree [-adf] [directory...]\n");
    exit();
}
int main(int argc, char **argv)
{
    int i;
    ARGBEGIN
    {
    default:
        usage();
    case 'a':
    case 'd':
    case 'f':
        flag[(u_char)ARGC()]++;
        break;
    }
    ARGEND

    if (argc == 0)
    {
        char current[1024];
        syscall_get_relative_path(current);
        tree(current);
        if (flag['d'])
        {
            printf("\n%d directories\n", dircnt);
        }
        else
        {
            printf("\n%d directories, %d files\n", dircnt, filecnt);
        }
    }
    else
    {
        for (int j = 0; j < argc; j++)
        {
            
            struct Env *env;
            env = envs + ENVX(syscall_getenvid());
            char tmp[1024];
            int i = 0;
            while ((env->r_path)[i] != 0)
            {
                tmp[i] = (env->r_path)[i];
                i++;
            }
            tmp[i] = 0;
            int len1 = strlen(tmp);
            int len2 = strlen(argv[j]);
            printf("len2 %d\n",len2);
            if (len1 == 1)
            {
                strcpy(tmp + 1, argv[j]);
            }
            else
            {
                tmp[len1] = '/';
                strcpy(tmp + len1 + 1, argv[j]);
                tmp[len1 + 1 + len2] = '\0';
            }
            tree(tmp);
            if (flag['d'])
            {
                printf("\n%d directories\n", dircnt);
            }
            else
            {
                printf("\n%d directories, %d files\n", dircnt, filecnt);
            }
        }
    }
    printf("\n");
    return 0;
}
