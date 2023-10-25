#include <lib.h>

int touch(const char *path)
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
    int len2 = strlen(path);
    if (len1 == 1)
    {
        strcpy(tmp + 1, path);
    }
    else
    {
        tmp[len1] = '/';
        strcpy(tmp + len1 + 1, path);
        tmp[len1 + 1 + len2] = '\0';
    }
    int r;
    if ((r = open(tmp, O_CREAT | FTYPE_REG)) > 0)
    {
        user_panic("touch: path %s already exist!\n", path);
    }
    if (r < 0)
    {
        user_panic("touch %s: %d\n", path, r);
    }
    return r;
}
int main(int argc, char **argv)
{
    touch(argv[1]);
}