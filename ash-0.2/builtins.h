/*
 * This file was generated by the mkbuiltins program.
 */

#include <sys/cdefs.h>
#define BLTINCMD 0
#define BREAKCMD 1
#define CDCMD 2
#define DOTCMD 3
#define ECHOCMD 4
#define EVALCMD 5
#define EXECCMD 6
#define EXITCMD 7
#define EXPORTCMD 8
#define FALSECMD 9
#define GETOPTSCMD 10
#define HASHCMD 11
#define JOBIDCMD 12
#define JOBSCMD 13
#define LCCMD 14
#define LOCALCMD 15
#define PWDCMD 16
#define READCMD 17
#define RETURNCMD 18
#define SETCMD 19
#define SETVARCMD 20
#define SHIFTCMD 21
#define TESTCMD 22
#define TRAPCMD 23
#define TRUECMD 24
#define UMASKCMD 25
#define UNSETCMD 26
#define WAITCMD 27

struct builtincmd {
      char *name;
      int code;
};

extern int (*const builtinfunc[])();
extern const struct builtincmd builtincmd[];
