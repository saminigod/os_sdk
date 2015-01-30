#define JICAMA
#define Extern extern
#include <sys/types.h>
#include <errno.h>
#include <setjmp.h>
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netbas/dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <netbas/netbas.h>
#include <fcntl.h>

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

static int __do_dir(const char *sDir, int);
static int __ps();
static int __klogdump();
static int __dll();
static int __dll();




#include "minix.h"

#define SIZE (16*1024)

static char s_proc_buffer[SIZE];
int dos_exec(long runp, char *buf, int sz);
static char cur_dir_str[PATH_MAX + 1];
static char *errmsg = "pwd: cannot search some directory on the path\n";
_PROTOTYPE(int kill_main, (int argc, char **argv));
_PROTOTYPE(int cat_main, (int argc, char **argv));
_PROTOTYPE(int echo_main, (int argc, char **argv));
_PROTOTYPE(int sleep_main, (int argc, char **argv));

static int __do_dir(const char *sDir, int dirmode)
{
	DIR* dosdir;

	fat_lfn_find_t *Entry;
	
   if(!sDir){
      return -1;
   }

   dosdir = netbas_opendir(sDir);

   //printf("try open dir %s ..\n", sDir);

   if (dosdir==NULL)
   {
	   printf( sDir);
	   printf("not found!\n");
	   return -1;
   }

   while (netbas_readdir(dosdir)!=NULL)
   {
	Entry=&dosdir->d_lfn;
	union dos_time t;
	union dos_date d;

	  d.d2=(unsigned short)((Entry->MTime>>16)&0xffff);
	  t.t2=(unsigned short)(Entry->MTime&0xffff);

	  if(dirmode){

 	 printf("%04d-%02d-%02d   %02d:%02d\t",
		d.d1.year + 1980,
        d.d1.month,
		d.d1.day_of_month, 
		t.t1.hour,
		t.t1.min
		//t.t1.sec * 2
		 );

	  }

	  if(MSDOS_ISDIR(Entry->Attr)){
		if(dirmode)
 	 		printf("%-10s ", "<DIR>");
			textcolor(YELLOW | BG_BLACK);
			printf(" %-14s", Entry->LName);
			textcolor(WHITE | BG_BLACK);
			if(dirmode){printf("\n");}
			else{printf("\t");}
	  }else{
		  if(dirmode)
	 		printf("%10d ", Entry->SizeLo);
			printf(" %-14s", Entry->LName);
			if(dirmode){printf("\n");}
			else{printf("\t");}
	  }

	}

   netbas_closedir(dosdir);
   if(!dirmode){printf("\n");}
   return 0;
}

int inkmodcmd(int argc, char *argv[])
{
	if (argc<2)
	{
		printf("useage: inskmod modname\n");
		return 1;
	}

	return netbas_module_in(argv[1], argv);
}

int rmkmodcmd(int argc, char *argv[])
{
	if (argc<2)
	{
		printf("useage: rmkmod modname\n");
		return 1;
	}

	return netbas_module_out(argv[1]);
}

#define SIZE_echo 1024
static char buf_echo[SIZE_echo];
static int count;

_PROTOTYPE(static void collect, (char *s));

void strmove(char *p, int chars)
{
	int i=0;
	while (p[i])
	{
		p[i] = p[i+chars];
		i++;
	}
	p[i] = 0;
}

void string_trim(char *str)
{
	char *p = str;
	int i = 0, j = 0;
	char *p2 =  str;

	if (!p)
	{
		return;
	}

	while (p[i])
	{
		/*if (p[i] == '/'){
			i = 1;
			p = &p[i];
		}*/
		if (p[i] == '\t')
		{
			strmove(&p[i], 1);
		}
		if (p[i] == '\b')
		{
			strmove(&p[i], 1);
			if(i>1)
				strmove(&p[i-1], 1);
		}
		i++;
	}
}



int mkdircmd(int argc, char *argv[])
{
	int argc=0;
	
	while (1){
		if(!argv[++argc])break;
		mkdir(argv[argc],0777);
	}

	return 0;
}

int rmcmd(int argc, char *argv[])
{
	int argc=0;
	
	while (1){
		if(!argv[++argc])break;
		printf("unlink %s\n", argv[argc]);
		unlink(argv[argc]);
	}

	return 0;
}


int rmdircmd(int argc, char *argv[])
{
	int i=0;
	
	while (i<argc){
		rmdir(argv[argc]);
	}

	return 0;
}


int printenvcmd(int argc, char *argv[])
{
	char **sptr;
	
	return(0);
}


int clearcmd(int argc, char *argv[])
{
	clear_screen();	
	return(0);
}


int pscmd(int argc, char *argv[])
{
	__ps();	
	return(0);
}

int lsproccmd(int argc, char *argv[])
{
	sh_lsproc(1, argv);	
	return(0);
}

int catproccmd(int argc, char *argv[])
{
	if (!argv[1])
	{
		return 1;
	}
	sh_proc_read(2, argv);	
	return(0);
}

int wrtproccmd(int argc, char *argv[])
{
	if (!argv[1])
	{
		return 1;
	}
	sh_proc_write(3, argv);	
	return(0);
}

int dodllinfo(int argc, char *argv[])
{
	__dll();	
	return(0);
}


int doklog(int argc, char *argv[])
{
	__klogdump();	
	return(0);
}



int lscmd(int argc, char *argv[])
{
	register char *cp, *er;
	char mypwd[PATH_MAX];

	if ((cp = argv[1]) == NULL){
		//er = ": no home directory";
		getcwd(mypwd, PATH_MAX);
		if(!mypwd[0])
		  cp="/";
	  else
		  cp = mypwd;
	}
	
	if(__do_dir(cp,0) < 0){
		er = ": bad directory";
	}
	else{
		return(0);
	}
		
	printf(cp != NULL? cp: "cd");
	err(er);
}

int dircmd(int argc, char *argv[])
{
	register char *cp, *er;
	char mypwd[PATH_MAX];


	if ((cp = argv[1]) == NULL){
		//er = ": no home directory";
		getcwd(mypwd, 64);
		if(!mypwd[0])
		  cp="/";
	  else
		  cp = mypwd;
	}
	
	if(__do_dir(cp,1) < 0){
		er = ": bad directory";
	}
	else{
		return(0);
	}
		
	printf(cp != NULL? cp: "cd");
	err(er);

}

int sh_lsproc(int argc, char *argv[])
{
	int e;

	memset(s_proc_buffer,0,sizeof(s_proc_buffer));
	e = ptrace((int)6, (pid_t)s_proc_buffer, (void*)SIZE, NULL);
	if (e < 0)
		return 1;
	printf("%s", s_proc_buffer);
	return 0;
}

int sh_proc_read(int argc, char *argv[])
{
	int e;
	char *proc_name = argv[1];

	memset(s_proc_buffer,0,sizeof(s_proc_buffer));
	//printf("proc is %s\n", proc_name);
	e = ptrace((int)7, (pid_t)s_proc_buffer, (void*)SIZE, proc_name);
	if (e < 0)
		return 1;
	printf("%s", s_proc_buffer);
	return 0;
}

int sh_proc_write(int argc, char *argv[])
{
	int e;
	char *proc_name = argv[1];

	memset(s_proc_buffer,0,sizeof(s_proc_buffer));

	sprintf(s_proc_buffer, "%s", argv[2]);
	e = ptrace((int)8, (pid_t)s_proc_buffer, (void*)SIZE, proc_name);
	if (e < 0)
		return 1;
	return 0;
}


static int __ps()
{
	int e;

	memset(s_proc_buffer,0,sizeof(s_proc_buffer));

	e = ptrace((int)0, (pid_t)s_proc_buffer, (void*)SIZE,NULL);
	if (e < 0)
		return 1;
	printf(s_proc_buffer);
	return 0;
}

static int __dll()
{
	int e;

	memset(s_proc_buffer,0,sizeof(s_proc_buffer));

	e = ptrace((int)2, (pid_t)s_proc_buffer, (void*)SIZE,NULL);
	if (e < 0)
		return 1;
	printf(s_proc_buffer);
	return 0;
}

static int __ks()
{
	int e;

	memset(s_proc_buffer,0,sizeof(s_proc_buffer));
	e = ptrace((int)1, (pid_t)s_proc_buffer, (void*)SIZE, NULL);
	if (e < 0)
		return 1;
	printf(s_proc_buffer);
	return 0;
}

static int __klogdump()
{
	int e;

	memset(s_proc_buffer,0,sizeof(s_proc_buffer));

	e = ptrace((int)5, (pid_t)s_proc_buffer, (void*)SIZE,NULL);
	if (e < 0)
		return 1;
	printf(s_proc_buffer);
	return 0;
}

static int __pause()
{
	printf("Press any key to continue ...");
	getchar();
   return 0;
}


int __exec_8086(char* fname)
{
	int fd;
	char *bptr;
	int dz;

	fd=open(fname, O_RDONLY);
	if(fd<0)return 1;

	//dz=flielength(fd);

	bptr=malloc(dz);
	read(fd,bptr,dz);
	close(fd);
	dos_exec(0x100, bptr,dz);
	return 0;
}



int __reboot(int argc, char *argv[])
{
	int flag;
	char *prog;

	/* Try to run the real McCoy. */

	//execv("/bin/halt.exe", argv);

	if ((prog = strrchr(*argv,'/')) == 0) prog= argv[0]; else argv++;

	sleep(2);	/* Not too fast. */

	reboot(strcmp(prog, "reboot") == 0 ? RBT_REBOOT : RBT_HALT);

	write(2, "reboot call failed\n", 19);
	return 1;
}


void
err(s)
char *s;
{
	if (*s)
		write(2, s, strlen(s));
}


int getcwdcmd(int argc, char *argv[])
{
  char *p;
  size_t n;

  p = getpwd(cur_dir_str, PATH_MAX);
  p=cur_dir_str;
  if (p == NULL) {
	write(2, errmsg, strlen(errmsg));
	exit(FAILURE);
  }
  n = strlen(p);
  p[n] = '\n';
  if (write(1, p, n + 1) != n + 1) 	exit(FAILURE);
  return(SUCCESS);
}

int finito(int a, char **b)
{ exit(0); }

int sh_mkdir(int argc, char** argv)
{
	if(argc !=2 ) {
		//printf("usage: mkdir dir_name. error: arg=%d\n", argc);
		return -1;
	}
	return mkdir(argv[1], 0);
}

int chroot_cmd(int argc, char** argv)
{
	if(argc !=2 ) {
		//printf("usage: mkdir dir_name. error: arg=%d\n", argc);
		return -1;
	}
	return chroot(argv[1]);
}

int sh_rmdir(int argc, char** argv)
{
	if(argc !=2 ) {
		//printf("usage: mkdir dir_name. error: arg=%d\n", argc);
		return -1;
	}
	return remove(argv[1]);
}

int sh_rename(int argc, char** argv)
{
	if (argc==3)
	{
		rename(argv[1], argv[2]);
	}else{
		printf("args error!\n");
	}
	return 0;
}

void std_err(s)
char *s;
{
  register char *p = s;

  while (*p != 0) p++;
 // write(2, s, (int) (p - s));
}

#define CHUNK_SIZE	4096

static int unbuffered;
static char ibuf[CHUNK_SIZE];
static char obuf[CHUNK_SIZE];
static char *op = obuf;

_PROTOTYPE(int main, (int argc, char **argv));
_PROTOTYPE(void copyfile, (int ifd, int ofd));
_PROTOTYPE(void flush, (void));
_PROTOTYPE(void fatal, (void));

int excode = 0;
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

int catcmd(argc, argv)
int argc;
char *argv[];
{
  int i, fd;

  memset(ibuf, 0, sizeof(ibuf));
  memset(obuf, 0, sizeof(obuf));
  unbuffered = 0;
  op = obuf;

  i = 1;
  /* Check for the -u flag -- unbuffered operation. */
  if (argc >= 2 && argv[1][0] == '-' && argv[1][1] == 'u' && argv[1][2] == 0) {
	unbuffered = 1;
	i = 2;
  }
  if (i >= argc) {
	copyfile(STDIN_FILENO, STDOUT_FILENO);
  } else {
	for ( ; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] == 0) {
			copyfile(STDIN_FILENO, STDOUT_FILENO);
		} else {
			fd = open(argv[i], O_RDONLY);
			if (fd < 0) {
				std_err("cat: cannot open ");
				std_err(argv[i]);
				std_err(": ");
				std_err(strerror (errno));
				std_err("\n");
				excode = 1;
			} else {
				copyfile(fd, STDOUT_FILENO);
				close(fd);
			}
		}
	}
  }
  flush();
  return(excode);
}

void copyfile(ifd, ofd)
int ifd, ofd;
{
  int n;

  while (1) {
	n = read(ifd, ibuf, CHUNK_SIZE);
	//if (n < 0) fatal();
	//if (n == 0) return;
	if (n <= 0) return;
	if (unbuffered || (op == obuf && n == CHUNK_SIZE)) {
		if (write(ofd, ibuf, n) != n) fatal();
	} else {
		int bytes_left;

		bytes_left = &obuf[CHUNK_SIZE] - op;
		if (n <= bytes_left) {
			memcpy(op, ibuf, (size_t)n);
			op += n;
		} else {
			memcpy(op, ibuf, (size_t)bytes_left);
			if (write(ofd, obuf, CHUNK_SIZE) != CHUNK_SIZE)
				fatal();
			n -= bytes_left;
			memcpy(obuf, ibuf + bytes_left, (size_t)n);
			op = obuf + n;
		}
	}
  }
}

void flush()
{
  if (op != obuf)
	if (write(STDOUT_FILENO, obuf, (size_t) (op - obuf)) <= 0) fatal();
}

static void fatal()
{
  perror("cat");
  exit(1);
}


#include <signal.h>


int killcmd(argc, argv)
int argc;
char **argv;
{
  pid_t proc;
  int ex = 0, signal = SIGTERM;
  char *end;
  long l;
  unsigned long ul;

  if (argc < 2){
	  kill_usage();
	  return 1;
	}
  if (argc > 1 && *argv[1] == '-') {
	ul = strtoul(argv[1] + 1, &end, 10);
	if (end == argv[1] + 1 || *end != 0 || ul > _NSIG) {kill_usage();return 1;}
	signal = ul;
	argv++;
	argc--;
  }
  while (--argc) {
	argv++;
	l = strtoul(*argv, &end, 10);
	if (end == *argv || *end != 0 || (pid_t) l != l)  {kill_usage();return 1;}
	proc = l;
	if (kill(proc, signal) < 0) {
		fprintf(stderr, "kill: %d: %s\n", proc, strerror(errno));
		ex = 1;
	}
  }
  return(ex);
}

static void kill_usage()
{
  fprintf(stderr, "Usage: kill [-sig] pid\n");
 // exit(1);
}



int sleepcmd(argc, argv)
int argc;
char *argv[];
{
  register seconds;
  register char c;

  seconds = 0;

  if (argc != 2) {
	std_err("Usage: sleep time\n");
	return (1);
  }
  while (c = *(argv[1])++) {
	if (c < '0' || c > '9') {
		std_err("sleep: bad arg\n");
		return (1);
	}
	seconds = 10 * seconds + (c - '0');
  }

  /* Now sleep. */
 // sleep(seconds);
 thread_msleep(seconds*1000);
  return(0);
}

int affirmative(void)
/* Get a yes/no answer from the suspecting user. */
{
	int c;
	int ok;

	fflush(stdout);
	fflush(stderr);

	while ((c= getchar()) == ' ') {}
	ok= (c == 'y' || c == 'Y');
	while (c != EOF && c != '\n') c= getchar();

	return ok;
}
#define BUF_SIZE 4096
static char cp_buf[BUF_SIZE];

int
cpcmd(int argc, char** argv)
{
	int in, out;
	char* src = argv[1];
	char* dst = argv[2];
	char *flags;
/* Call name of this program. */
	char *prog_name= basename(argv[0]);
	if(argc < 2) {
		printf("usage: cp src dest.\n");
		exit(0);
	}

	/* Required action. */
	if (strcmp(prog_name, "cp") == 0) {
		flags= "pifsmrRvx";
		if(argc < 3) {
			printf("usage: cp src dest.\n");
			exit(0);
		}

		in = open(src, O_RDONLY);
		if(in == -1) {
			printf("open src failed.\n");
			return;
		}

		out = creat(dst, 0666);
		if(out == -1) {
			printf("open dest failed.\n");
			return;
		}

		while(1) {
			int rb = read(in, cp_buf, BUF_SIZE);

			if(rb <= 0) {
				//printf("done.\n");
				break;
			}

			if(write(out, cp_buf, rb) < 0) {
				printf("write failed\n");
				break;
			}
		}
		
		close(in);
		close(out);
	} else
	if (strcmp(prog_name, "mv") == 0) {
		flags= "ifsmvx";
		unlink(argv[1]);
	} else
	if (strcmp(prog_name, "rm") == 0) {
		flags= "ifrRvx";
		fprintf(stderr, "Remove %s? ", argv[1]);
		unlink(argv[1]);
	} 
	else{
		printf("%s:command not found\n", prog_name);
	}



}


