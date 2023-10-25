#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"
// 光标向左移动y个
#define toLeft(y) printf("\033[%dD", (y))
// 光标向右移动y个
#define toRight(y) printf("\033[%dC", (y))

#define toDown(y) printf("\033[%dB", (y))
#define toUp(y) printf("\033[%dA", (y))
static int init;
static int historeLength;
static int off[128];

/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */
int _gettoken(char *s, char **p1, char **p2)
{
	*p1 = 0;
	*p2 = 0;
	if (s == 0)
	{
		return 0;
	}

	while (strchr(WHITESPACE, *s))
	{
		*s++ = 0;
	}
	if (*s == 0)
	{
		return 0;
	}
	if (strchr(SYMBOLS, *s))
	{
		int t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		return t;
	}
	if (*s == '\"')
	{
		s++;
		*p1 = s;
		while (*s != '\"')
			s++;
		*p2 = s;
		*s++ = 0;
		return 'w';
	}
	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s))
	{
		s++;
	}
	*p2 = s;
	return 'w';
}

int gettoken(char *s, char **p1)
{
	static int c, nc;
	static char *np1, *np2;

	if (s)
	{
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 128

int parsecmd(char **argv, int *rightpipe)
{
	int argc = 0;
	while (1)
	{
		char *t;
		int i;
		int fd, r;
		int c = gettoken(0, &t);
		switch (c)
		{
		case 0:
			return argc;
		case 'w':
			if (argc >= MAXARGS)
			{
				debugf("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;
		case '<':
			if (gettoken(0, &t) != 'w')
			{
				debugf("syntax error: < not followed by word\n");
				exit();
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			/* Exercise 6.5: Your code here. (1/3) */
			r = open(t, O_RDONLY);
			if (r < 0)
			{
				user_panic("!!");
			}
			dup(r, 0);
			close(r);
			// user_panic("< redirection not implemented");

			break;
		case '>':
			if (gettoken(0, &t) != 'w')
			{
				debugf("syntax error: > not followed by word\n");
				exit();
			}
			// Open 't' for writing, dup it onto fd 1, and then close the original fd.
			/* Exercise 6.5: Your code here. (2/3) */
			r = open(t, O_WRONLY);
			if (r < 0)
			{
				// user_panic("2222");
				char relativePath[1024];
				syscall_get_relative_path(relativePath);
				if (strlen(relativePath) != 1)
				{
					strcpy(relativePath + strlen(relativePath), "/");
				}
				strcpy(relativePath + strlen(relativePath), t);
				open(relativePath, O_CREAT | FTYPE_REG);
			}
			dup(r, 1);
			close(r);
			// user_panic("> redirection not implemented");

			break;
		case ';':
			*rightpipe = fork();
			if (*rightpipe == 0)
				return argc;
			else
			{
				wait(*rightpipe);
				return parsecmd(argv, rightpipe);
			}
			break;
		case '&':
			r = fork();
			if (r == 0)
				return argc;
			else
				return parsecmd(argv, r);
			break;
		case '|':;
			/*
			 * First, allocate a pipe.
			 * Then fork, set '*rightpipe' to the returned child envid or zero.
			 * The child runs the right side of the pipe:
			 * - dup the read end of the pipe onto 0
			 * - close the read end of the pipe
			 * - close the write end of the pipe
			 * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
			 *   command line.
			 * The parent runs the left side of the pipe:
			 * - dup the write end of the pipe onto 1
			 * - close the write end of the pipe
			 * - close the read end of the pipe
			 * - and 'return argc', to execute the left of the pipeline.
			 */
			int p[2];
			/* Exercise 6.5: Your code here. (3/3) */
			if ((r = pipe(p)) < 0)
			{
				user_panic("!!");
			}
			*rightpipe = fork();
			if (*rightpipe < 0)
			{
				user_panic("!!");
			}
			if (*rightpipe == 0)
			{
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv, rightpipe);
			}
			else
			{
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				return argc;
			}
			// user_panic("| not implemented");

			break;
		}
	}

	return argc;
}
void save(const char *s)
{
	int r;
	int fd;

	if (!init)
	{
		if (open("/.history", FTYPE_REG | O_CREAT) < 0)
		{
			debugf("create .history failed\n");
			exit();
		}
	}

	if ((r = open("/.history", O_WRONLY | O_APPEND)) < 0)
	{
		debugf("can't open file .history: %d\n", r);
		exit();
	}
	fd = r;

	if ((r = write(fd, s, strlen(s))) != strlen(s))
	{
		user_panic("write error .history: %d\n", r);
	}

	// it is need to write a '\n'
	if ((r = write(fd, "\n", 1)) != 1)
	{
		user_panic("write error .history: %d\n", r);
	}

	if (!init)
	{
		init = 1;
		off[historeLength++] = strlen(s) + 1;
	}
	else
	{
		off[historeLength] = off[historeLength - 1] + strlen(s) + 1;
		historeLength++;
	}

	close(fd);
}
void change(int idx, char *cmd)
{
	int r;
	int fd;
	char buf[4096];
	int len = 0;

	if ((r = open("/.history", O_RDONLY)) < 0)
	{
		debugf("can't open file .history: %d\n", r);
		exit();
	}
	fd = r;

	if (idx != 0)
	{
		if ((r = readn(fd, buf, off[idx - 1])) != off[idx - 1])
		{
			user_panic("can't read file .history: %d", r);
		}
		len = off[idx] - off[idx - 1];
	}
	else
	{
		len = off[idx];
	}

	if ((r = readn(fd, cmd, len)) != len)
	{
		user_panic("can't read file .history: %d", r);
	}

	close(fd);

	cmd[len - 1] = 0;
}
void runcmd(char *s)
{
	gettoken(s, 0);

	char *argv[MAXARGS];
	int rightpipe = 0;
	int argc = parsecmd(argv, &rightpipe);
	if (argc == 0)
	{
		return;
	}
	argv[argc] = 0;
	int child = spawn(argv[0], argv);
	close_all();
	if (child >= 0)
	{
		wait(child);
	}
	else
	{
		debugf("spawn %s: %d\n", argv[0], child);
	}
	if (rightpipe)
	{
		wait(rightpipe);
	}
	exit();
}

static int k;
static char currentcmd[1024];

void readline(char *buf, u_int n)
{
	int idx = k;
	int r;
	int len = 0;
	int i = 0;
	char temp;
	while (i < n)
	{
		if ((r = read(0, &temp, 1)) != 1)
		{
			if (r < 0)
			{
				debugf("read error: %d\n", r);
			}
			exit();
		}
		if (temp == '\b' || temp == 0x7f)
		{
			if (i > 0)
			{
				if (i == len)
				{
					i--;
					buf[i] = 0;
					toLeft(1);
					printf(" ");
					toLeft(1);
				}
				else
				{
					for (int j = i - 1; j < len - 1; j++)
					{
						buf[j] = buf[j + 1];
					}
					buf[len - 1] = 0;
					toLeft(i);
					i--;
					printf("%s ", buf);
					toLeft(len - i);
				}
				len -= 1;
			}
		}
		else if (temp == '\033')
		{
			char h;
			read(0, &h, 1);
			if (h == '[')
			{
				read(0, &h, 1);
				switch (h)
				{
				case 'A':
					toDown(1);
					if (idx == historeLength)
					{
						buf[len] = 0;
						strcpy(currentcmd, buf);
					}
					if (idx > 0)
					{
						idx--;
					}
					if (i > 0)
					{
						toLeft(i);
					}
					change(idx, buf);
					printf("%s", buf);
					if (strlen(buf) < len)
					{
						for (int j = 0; j < len - strlen(buf); j++)
						{
							printf(" ");
						}
						printf("\033[%dD", len - strlen(buf));
					}
					len = i = strlen(buf);
					break;

				case 'B':
					if (idx == historeLength - 1)
					{
						idx++;
						strcpy(buf, currentcmd);
					}
					else if (idx == historeLength)
					{
						break;
					}
					else
					{
						idx++;
						change(idx, buf);
					}
					if (i > 0)
					{
						toLeft(i);
					}
					printf("%s", buf);
					if (strlen(buf) < len)
					{
						for (int j = 0; j < len - strlen(buf); j++)
						{
							printf(" ");
						}
						printf("\033[%dD", len - strlen(buf));
					}
					len = i = strlen(buf);
					break;
				case 'C':
					if (i < len)
					{
						i += 1;
					}
					else
					{
						toLeft(1);
					}
					break;
				case 'D':
					if (i > 0)
					{
						i -= 1;
					}
					else
					{
						toRight(1);
					}
					break;
				default:
					break;
				}
			}
		}
		else if (temp == '\r' || temp == '\n')
		{

			buf[len] = 0;
			return;
		}
		else
		{
			if (i == len)
			{
				buf[i++] = temp;
			}
			else
			{
				for (int j = len; j > i; j--)
				{
					buf[j] = buf[j - 1];
				}
				buf[i] = temp;
				buf[len + 1] = 0;
				toLeft(++i);
				printf("%s", buf);
				toLeft(len - i + 1);
			}
			len += 1;
		}

		if (len >= n)
		{
			break;
		}
	}
	debugf("line too long\n");
	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n')
	{
		;
	}
	buf[0] = 0;
}

char buf[1024];

void usage(void)
{
	debugf("usage: sh [-dix] [command-file]\n");
	exit();
}

int main(int argc, char **argv)
{
	int r;
	int interactive = iscons(0);
	int echocmds = 0;
	debugf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	debugf("::                                                         ::\n");
	debugf("::                     MOS Shell 2023                      ::\n");
	debugf("::                                                         ::\n");
	debugf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	ARGBEGIN
	{
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1)
	{
		usage();
	}
	if (argc == 1)
	{
		close(0);
		if ((r = open(argv[1], O_RDONLY)) < 0)
		{
			user_panic("open %s: %d", argv[1], r);
		}
		user_assert(r == 0);
	}
	for (;;)
	{
		if (interactive)
		{
			printf("\n$ ");
		}
		readline(buf, sizeof buf);
		save(buf);
		if (buf[0] == '#')
		{
			continue;
		}
		if (echocmds)
		{
			printf("# %s\n", buf);
		}
		if (buf[0] == 'p' && buf[1] == 'w' && buf[2] == 'd')
		{
			char cur[1024];
			syscall_get_relative_path(cur);
			printf("now you are in %s\n", cur);
			continue;
		}
		if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ')
		{
			int r;
			char cur[1024] = {0};
			char *m = (buf + 2);
			while (*m == ' ')
				m++;
			char *p = m;
			if (p[0] != '/')
			{
				int toFather = 0;
				if (p[0] == '.' && p[1] == '/')
				{
					p += 2;
				}
				if (p[0] == '.' && p[1] == '.')
				{
					if (p[2] == '/')
						p += 3;
					else
					{
						char m[1024] = {0};
						strcpy(p, m);
					}
					toFather = 1;
				}
				syscall_get_relative_path(cur);
				if (toFather == 1)
				{
					int k;
					for (k = strlen(cur) - 1; cur[k] != '/'; k--)
						;
					cur[k + 1] = 0;
				}
				int len1 = strlen(cur);
				int len2 = strlen(p);
				if (len1 == 1)
				{
					strcpy(cur + 1, p);
				}
				else
				{
					cur[len1] = '/';
					strcpy(cur + len1 + 1, p);
					cur[len1 + 1 + len2] = '\0';
				}
			}
			else
			{
				strcpy(cur, argv[1]);
			}
			printf("cur:%s\n", cur);
			struct Stat st;
			if ((r = stat(cur, &st)) < 0)
			{
				printf("wrong path\n");
				printf("4");
				continue;
			}
			if (!st.st_isdir)
			{
				printf("%s is not a directory\n", cur);
				printf("5");
				continue;
			}
			if ((r = syscall_set_relative_path(cur)) < 0)
			{
				printf("error\n");
				continue;
			}
			struct Env *env;
			env = envs + ENVX(syscall_getenvid());
			continue;
		}
		if ((r = fork()) < 0)
		{
			user_panic("fork: %d", r);
		}
		if (r == 0)
		{
			runcmd(buf);
			exit();
		}
		else
		{
			wait(r);
		}
	}
	return 0;
}
