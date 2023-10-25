#include <lib.h>

void usage(void) 
{
	printf("usage: history\n");
    exit();
}

int main(int argc, char **argv) {
    int r;
    int f;
    int line = 0;
    int another = 1;
    char buf;

    if (argc != 1) {
        usage();
    } else {
        if ((r = open(".history", O_RDONLY)) < 0) {
            user_panic("can't open file .history: %d\n", r);
        }
        f = r;

        while ((r = read(f, &buf, 1)) == 1) {
            if (another) {
                another = 0;
            } 
            if (buf == '\n') {
                another = 1;
            }
        }

    }

    return 0;
}