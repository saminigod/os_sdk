/*
 * This file was generated by the mkbuiltins program.
 */

#include "shell.h"
#include "builtins.h"

int bltincmd();
int breakcmd();
int cdcmd();
int dotcmd();
int echocmd();
int evalcmd();
int execcmd();
int exitcmd();
int exportcmd();
int falsecmd();
int getoptscmd();
int hashcmd();
int jobidcmd();
int jobscmd();
int lccmd();
int localcmd();
int pwdcmd();
int readcmd();
int returncmd();
int setcmd();
int setvarcmd();
int shiftcmd();
int testcmd();
int trapcmd();
int truecmd();
int umaskcmd();
int unsetcmd();
int waitcmd();


int unamecmd();
int touchcmd();
int dircmd();
int lscmd();
int inkmodcmd();
int rmkmodcmd();
int mkdircmd();
int rmdircmd();
int rmcmd();
int catcmd();
int printenvcmd();
int catcmd();
int pscmd();
int killcmd();
int sleepcmd();
int getcwdcmd();
int inkmodcmd();
int rmkmodcmd();
int clearcmd();
int lsproccmd();
int catproccmd();
int wrtproccmd();
int cpcmd();

int (*const builtinfunc[])() = {
	bltincmd,
	breakcmd,
	cdcmd,
	dotcmd,
	echocmd,
	evalcmd,
	execcmd,
	exitcmd,
	exportcmd,
	falsecmd,
	getoptscmd,
	hashcmd,
	jobidcmd,
	jobscmd,
	lccmd,
	localcmd,
	pwdcmd,
	readcmd,
	returncmd,
	setcmd,
	setvarcmd,
	shiftcmd,
	testcmd,
	trapcmd,
	truecmd,
	umaskcmd,
	unsetcmd,
	waitcmd,	//27

	unamecmd, //28(CMD_BASE)
	touchcmd, //29
	dircmd, //30
	lscmd, //31
	inkmodcmd, //32
	rmkmodcmd, //33
	pscmd, //34
	catcmd, //35
	killcmd, //36
	sleepcmd, //37
	mkdircmd, //38
	rmdircmd, //39
	printenvcmd,//40
	clearcmd, //41
	lsproccmd, //42
	catproccmd, //43
	wrtproccmd, //44
	cpcmd, //45
};

#define CMD_BASE 28
const struct builtincmd builtincmd[] = {
	"command", 0,
	"break", 1,
	"continue", 1,
	"cd", 2,
	"chdir", 2,
	".", 3,
	"echo", 4,
	"eval", 5,
	"exec", 6,
	"exit", 7,
	"export", 8,
	"readonly", 8,
	"false", 9,
	"getopts", 10,
	"hash", 11,
	"jobid", 12,
	"jobs", 13,
	"lc", 14,
	"local", 15,
	"pwd", 16,
	"read", 17,
	"return", 18,
	"set", 19,
	"setvar", 20,
	"shift", 21,
	"test", 22,
	"[", 22,
	"trap", 23,
	":", 24,
	"true", 24,
	"umask", 25,
	"unset", 26,
	"wait", 27,

	"uname", CMD_BASE+0,//28
	"touch", CMD_BASE+1, 
	"dir",		CMD_BASE+2,
	"ls",		CMD_BASE+3,
	"cls",		CMD_BASE+13,
	"clear",		CMD_BASE+13,
	"lsproc", CMD_BASE+14,
	"proc", CMD_BASE+15,
	"wproc", CMD_BASE+16,
	"inkmod", CMD_BASE+4,
	"rmkmod", CMD_BASE+5,
	"uname", CMD_BASE+1,
	"cp", CMD_BASE+17,
	"rm", CMD_BASE,
	"mv", CMD_BASE,
	"ln", CMD_BASE,
	"cpdir", CMD_BASE,
	"clone", CMD_BASE,
	"ps", CMD_BASE+6,
	"kill", CMD_BASE+8,
	"cat", CMD_BASE+7,
	"mkdir", CMD_BASE+10,
	"rm", CMD_BASE+11,
	"rmdir", CMD_BASE+11,
	"sleep", CMD_BASE+9,
	"print", 4,
	"printenv", CMD_BASE+12,
	NULL, 0
};
