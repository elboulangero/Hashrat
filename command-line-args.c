#include "command-line-args.h"

#define CMDLINE_ARG_NAMEVALUE 1
#define CMDLINE_FROM_LISTFILE 2
#define CMDLINE_XATTR     4
#define CMDLINE_MEMCACHED 8

//this function is called when a command-line switch has been recognized. It's told which flags to set
//and whether the switch takes a string argument, which it then reads and stores in the variable list
//with the name supplied in 'VarName'. It blanks out anything it reads, so that only unrecognized 
//arguments remain, which are treated as filenames for processing

int CommandLineHandleArg(int argc, char *argv[], int pos, int ParseFlags, int SetFlags, char *VarName, char *VarValue, ListNode *Vars)
{
	Flags |= SetFlags;
	
	if (ParseFlags & CMDLINE_ARG_NAMEVALUE)
	{
	if (argv[pos + 1] ==NULL)
	{
		printf("ERROR: The %s option requires an argument.\n",argv[pos]);
		exit(1);
	}
	else
	{
		strcpy(argv[pos],"");
		pos++;
		if (SetFlags & FLAG_INCLUDE) AddIncludeExclude(FLAG_INCLUDE, argv[pos]);
		else if (SetFlags & FLAG_EXCLUDE) AddIncludeExclude(FLAG_EXCLUDE, argv[pos]);
		else SetVar(Vars,VarName,argv[pos]);
	}
	}
	else if (StrLen(VarName)) SetVar(Vars,VarName,VarValue);

	if (argc > 0) strcpy(argv[pos],"");

return(ParseFlags);
}



void CommandLineSetCtx(int argc, char *argv[], int i, HashratCtx *Ctx, int Flag)
{
Ctx->Flags |= Flag;
strcpy(argv[i],"");
}




void CommandLineHandleUpdate(int argc, char *argv[], int i, HashratCtx *Ctx)
{
char *Token=NULL, *ptr;

Flags |= FLAG_UPDATE;
strcpy(argv[i],"");
i++;
ptr=GetToken(argv[i],",",&Token,0);
while (ptr)
{
	if (strcasecmp(Token, "stderr")==0) Ctx->Aux=STREAMFromFD(2);
	else if (strcasecmp(Token, "xattr")==0) Ctx->Flags |= CTX_STORE_XATTR;
	else if (strcasecmp(Token, "memcached")==0) Ctx->Flags |= CTX_STORE_MEMCACHED;
	else if (strcasecmp(Token, "mcd")==0) Ctx->Flags |= CTX_STORE_MEMCACHED;
	else if (! Ctx->Aux) Ctx->Aux=STREAMOpenFile(Token,O_WRONLY | O_CREAT | O_TRUNC);

ptr=GetToken(ptr,",",&Token,0);
}
strcpy(argv[i],"");

DestroyString(Token);
}

//this is the main parsing function that goes through the command-line args
HashratCtx *CommandLineParseArgs(int argc,char *argv[])
{
int i;
char *ptr, *Tempstr=NULL;
HashratCtx *Ctx;
int ParseFlags=0;

//You never know when you're going to be run with  no args at all (say, out of inetd)

//Setup default context
Ctx=(HashratCtx *) calloc(1,sizeof(HashratCtx));
Ctx->Action=ACT_HASH;
Ctx->ListPath=CopyStr(Ctx->ListPath,"-");

if (argc < 1) 
{
	Ctx->Action=ACT_PRINTUSAGE;
	return(Ctx);
}

Ctx->Vars=ListCreate();
Ctx->Out=STREAMFromFD(1);
SetVar(Ctx->Vars,"HashType","md5");

//argv[0] might be full path to the program, or just its name
ptr=strrchr(argv[0],'/');
if (! ptr) ptr=argv[0];
else ptr++;


//if the program name is something other than 'hashrat', then we're being used as a drop-in
//replacement for another program. Change flags/behavior accordingly
if (strcmp(ptr,"md5sum")==0) 
ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "md5",Ctx->Vars);
if (
		(strcmp(ptr,"sha1sum")==0) ||
		(strcmp(ptr,"shasum")==0) 
	) 
ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "sha1",Ctx->Vars);
if (strcmp(ptr,"sha256sum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "sha256",Ctx->Vars);
if (strcmp(ptr,"sha512sum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "sha512",Ctx->Vars);
if (strcmp(ptr,"whirlpoolsum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "whirlpool",Ctx->Vars);
if (strcmp(ptr,"jh224sum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "jh-224",Ctx->Vars);
if (strcmp(ptr,"jh256sum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "jh-256",Ctx->Vars);
if (strcmp(ptr,"jh384sum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "jh-385",Ctx->Vars);
if (strcmp(ptr,"jh512sum")==0) ParseFlags |= CommandLineHandleArg(0, argv, i, 0, FLAG_TRAD_OUTPUT, "HashType", "jh-512",Ctx->Vars);


if (strcmp(ptr,"hashrat.cgi")==0) 
{
	Ctx->Action=ACT_CGI;
	return(Ctx);
}



//here we got through the command-line args, and set things up whenever we find one that we
//recognize. We blank the args we use so that any 'unrecognized' ones still left after this
//process can be treated as filenames for hashing
for (i=1; i < argc; i++)
{
if (
		(strcmp(argv[i],"--version")==0) ||
		(strcmp(argv[i],"-version")==0)
	)
{
	printf("version: %s\n",VERSION);
	return(NULL);
}
else if (
		(strcmp(argv[i],"--help")==0) ||
		(strcmp(argv[i],"-help")==0) ||
		(strcmp(argv[i],"-?")==0)
	)
{
	Ctx->Action=ACT_PRINTUSAGE;
	return(Ctx);
}
else if (strcmp(argv[i],"-c")==0)
{
	Ctx->Action = ACT_CHECK;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-cf")==0)
{
	Ctx->Action = ACT_CHECK;
	ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_OUTPUT_FAILS, "", "",Ctx->Vars);
}
else if (strcmp(argv[i],"-s")==0)
{
	Ctx->Action = ACT_SIGN;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-sign")==0)
{
	Ctx->Action = ACT_SIGN;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-cs")==0)
{
	Ctx->Action = ACT_CHECKSIGN;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-checksign")==0)
{
	Ctx->Action = ACT_CHECKSIGN;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-m")==0)
{
	Ctx->Action = ACT_FINDMATCHES;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-lm")==0)
{
	Ctx->Action = ACT_LOADMATCHES;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-hook")==0) 
{
	strcpy(argv[i],"");
	i++;
	DiffHook=CopyStr(DiffHook,argv[i]);
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-h")==0) 
{
	strcpy(argv[i],"");
	i++;
	DiffHook=CopyStr(DiffHook,argv[i]);
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-cgi")==0)
{
	Ctx->Action = ACT_CGI;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-md5")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "md5",Ctx->Vars);
else if (strcmp(argv[i],"-sha")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "sha1",Ctx->Vars);
else if (strcmp(argv[i],"-sha1")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "sha1",Ctx->Vars);
else if (strcmp(argv[i],"-sha256")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "sha256",Ctx->Vars);
else if (strcmp(argv[i],"-sha512")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "sha512",Ctx->Vars);
else if (strcmp(argv[i],"-whirl")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "whirlpool",Ctx->Vars);
else if (strcmp(argv[i],"-whirlpool")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "whirlpool",Ctx->Vars);
else if (strcmp(argv[i],"-jh-224")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-224",Ctx->Vars);
else if (strcmp(argv[i],"-jh-256")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-256",Ctx->Vars);
else if (strcmp(argv[i],"-jh-384")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-384",Ctx->Vars);
else if (strcmp(argv[i],"-jh-512")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-512",Ctx->Vars);
else if (strcmp(argv[i],"-jh224")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-224",Ctx->Vars);
else if (strcmp(argv[i],"-jh256")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-256",Ctx->Vars);
else if (strcmp(argv[i],"-jh384")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-384",Ctx->Vars);
else if (strcmp(argv[i],"-jh512")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-512",Ctx->Vars);
else if (strcmp(argv[i],"-jh")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "jh-512",Ctx->Vars);
//else if (strcmp(argv[i],"-crc32")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, 0, "HashType", "crc32",Ctx->Vars);
else if (strcmp(argv[i],"-8")==0)  CommandLineSetCtx(argc, argv, i, Ctx,  CTX_BASE8);
else if (strcmp(argv[i],"-10")==0) CommandLineSetCtx(argc, argv, i, Ctx,  CTX_BASE10);
else if (strcmp(argv[i],"-16")==0) CommandLineSetCtx(argc, argv, i, Ctx,  CTX_HEX);
else if (strcmp(argv[i],"-H")==0)  CommandLineSetCtx(argc, argv, i, Ctx,  CTX_HEXUPPER);
else if (strcmp(argv[i],"-HEX")==0) CommandLineSetCtx(argc, argv, i, Ctx,  CTX_HEXUPPER);
else if (strcmp(argv[i],"-64")==0) CommandLineSetCtx(argc, argv, i, Ctx,  CTX_BASE64);
else if (strcmp(argv[i],"-base64")==0) CommandLineSetCtx(argc, argv, i, Ctx,  CTX_BASE64);
else if (strcmp(argv[i],"-d")==0) CommandLineSetCtx(argc, argv, i, Ctx, CTX_DEREFERENCE);
else if (strcmp(argv[i],"-X")==0) CommandLineSetCtx(argc, argv, i, Ctx, CTX_EXES);
else if (strcmp(argv[i],"-exe")==0) CommandLineSetCtx(argc, argv, i, Ctx, CTX_EXES);
else if (strcmp(argv[i],"-n")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_ARG_NAMEVALUE, 0, "Output:Length", "",Ctx->Vars);
else if (strcmp(argv[i],"-hmac")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_ARG_NAMEVALUE, FLAG_HMAC, "EncryptionKey", "",Ctx->Vars);
else if (strcmp(argv[i],"-idfile")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_ARG_NAMEVALUE, 0,  "SshIdFile", "",Ctx->Vars);
else if (strcmp(argv[i],"-r")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_RECURSE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-f")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, CMDLINE_FROM_LISTFILE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-i")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_ARG_NAMEVALUE, FLAG_INCLUDE , "", "",Ctx->Vars);
else if (strcmp(argv[i],"-x")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_ARG_NAMEVALUE, FLAG_EXCLUDE , "", "",Ctx->Vars);
else if (strcmp(argv[i],"-dirmode")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_DIRMODE | FLAG_RECURSE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-devmode")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_DIRMODE | FLAG_DEVMODE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-lines")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_LINEMODE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-rawlines")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_RAW|FLAG_LINEMODE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-rl")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_RAW|FLAG_LINEMODE, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-fs")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_ONE_FS, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-xattr")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_XATTR, 0, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-strict")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_FULLCHECK, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-color")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_COLOR, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-S")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_FULLCHECK, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-net")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_NET, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-memcached")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_ARG_NAMEVALUE|CMDLINE_MEMCACHED, 0, "Memcached:Server", "",Ctx->Vars);
else if (strcmp(argv[i],"-mcd")==0) ParseFlags |= CommandLineHandleArg(argc, argv, i, CMDLINE_ARG_NAMEVALUE| CMDLINE_MEMCACHED, 0, "Memcached:Server", "",Ctx->Vars);
else if (
					(strcmp(argv[i],"-t")==0) ||
					(strcmp(argv[i],"-trad")==0)
				) ParseFlags |= CommandLineHandleArg(argc, argv, i, 0, FLAG_TRAD_OUTPUT, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-u")==0) CommandLineHandleUpdate(argc, argv, i, Ctx);
else if (strcmp(argv[i],"-attrs")==0) 
{
	strcpy(argv[i],"");
	i++;
	SetupXAttrList(argv[i]);
	strcpy(argv[i],"");
}


}

//The rest of this function finalizes setup based on what we parsed over all the command line


//if we're reading from a list file, then...
if ((ParseFlags & CMDLINE_FROM_LISTFILE))
{
	//... set appropriate action type
	if (Ctx->Action==ACT_HASH) Ctx->Action=ACT_HASH_LISTFILE;

	//find list file name on command line (will still be untouched as it wasn't
	//recognized as a flag in the above processing
	for (i=1; i < argc; i++)
	{
		if (StrLen(argv[i]))
		{
			Ctx->ListPath=CopyStr(Ctx->ListPath, argv[i]);
			strcpy(argv[i],"");
			break;
		}
	}
}




//if -hmac set, then upgrade hash type to the hmac version
if (Flags & FLAG_HMAC)
{
  Ctx->HashType=MCopyStr(Ctx->HashType,"hmac-",GetVar(Ctx->Vars,"HashType"),NULL);
  HMACSetup();
}
else Ctx->HashType=CopyStr(Ctx->HashType,GetVar(Ctx->Vars,"HashType"));

switch (Ctx->Action)
{
case ACT_CHECK:
	if (ParseFlags & CMDLINE_XATTR) Ctx->Action=ACT_CHECK_XATTR;
	if (ParseFlags & CMDLINE_MEMCACHED) Ctx->Action=ACT_CHECK_MEMCACHED;
break;

case ACT_HASH:
case ACT_HASH_LISTFILE:
	if (ParseFlags & CMDLINE_XATTR) Ctx->Flags |= CTX_STORE_XATTR;
	if (ParseFlags & CMDLINE_MEMCACHED) Ctx->Flags |= CTX_STORE_MEMCACHED;
break;

case ACT_FINDMATCHES:
	if (ParseFlags & CMDLINE_MEMCACHED) Ctx->Action=ACT_FINDMATCHES_MEMCACHED;
break;


}

//if no path given, then force to '-' for 'standard in'
ptr=GetVar(Ctx->Vars,"Path");
if (! StrLen(ptr)) SetVar(Ctx->Vars,"Path","-");

DestroyString(Tempstr);
return(Ctx);
}



void CommandLinePrintUsage()
{
printf("Hashrat: version %s\n",VERSION);
printf("Author: Colum Paget\n");
printf("Email: colums.projects@gmail.com\n");
printf("Blog:  http://idratherhack.blogspot.com\n");
printf("Credits:\n	Thanks to the people who invented the hash functions!\n	MD5: Ronald Rivest\n	Whirlpool: Vincent Rijmen, Paulo S. L. M. Barreto\n	JH: Hongjun Wu\n	SHA: The NSA (thanks, but please stop reading my email. It's kinda creepy.).\n\n Special thanks to Professor Hongjun Wu for taking the time to confirm that his JH algorithm is free for use in GPL programs.\n");

printf("\n");
printf("Usage:\n    hashrat [options] [path to hash]...\n");
printf("\n    hashrat -c [options] [input file of hashes]...\n\n");

printf("Options:\n");
printf("  %-15s %s\n","--help", "Print this help");
printf("  %-15s %s\n","-help", "Print this help");
printf("  %-15s %s\n","-?", "Print this help");
printf("  %-15s %s\n","--version", "Print program version");
printf("  %-15s %s\n","-version", "Print program version");
printf("  %-15s %s\n","-md5", "Use md5 hash algorithmn");
printf("  %-15s %s\n","-sha1", "Use sha1 hash algorithmn");
printf("  %-15s %s\n","-sha256", "Use sha256 hash algorithmn");
printf("  %-15s %s\n","-sha512", "Use sha512 hash algorithmn");
printf("  %-15s %s\n","-whirl", "Use whirlpool hash algorithmn");
printf("  %-15s %s\n","-whirlpool", "Use whirlpool hash algorithmn");
printf("  %-15s %s\n","-jh244", "Use jh-244 hash algorithmn");
printf("  %-15s %s\n","-jh256", "Use jh-256 hash algorithmn");
printf("  %-15s %s\n","-jh384", "Use jh-384 hash algorithmn");
printf("  %-15s %s\n","-jh512", "Use jh-512 hash algorithmn");
printf("  %-15s %s\n","-hmac", "HMAC using specified hash algorithm");
printf("  %-15s %s\n","-8", "Encode with octal instead of hex");
printf("  %-15s %s\n","-10", "Encode with decimal instead of hex");
printf("  %-15s %s\n","-H", "Encode with UPPERCASE hexadecimal");
printf("  %-15s %s\n","-HEX", "Encode with UPPERCASE hexadecimal");
printf("  %-15s %s\n","-64", "Encode with base65 instead of hex");
printf("  %-15s %s\n","-base64", "Encode with base65 instead of hex");
printf("  %-15s %s\n","-t", "Output hashes in traditional md5sum, shaXsum format");
printf("  %-15s %s\n","-trad", "Output hashes in traditional md5sum, shaXsum format");
printf("  %-15s %s\n","-r", "Recurse into directories when hashing files");
printf("  %-15s %s\n","-f <listfile>", "Hash files listed in <listfile>");
printf("  %-15s %s\n","-i <pattern>", "Only hash items matching <pattern>");
printf("  %-15s %s\n","-x <pattern>", "Exclude items matching <pattern>");
printf("  %-15s %s\n","-n <length>", "Truncate hashes to <length> bytes");
printf("  %-15s %s\n","-c", "CHECK hashes against list from file (or stdin)");
printf("  %-15s %s\n","-cf", "CHECK hashes but only show failures");
printf("  %-15s %s\n","-m", "MATCH files from a list read from stdin.");
printf("  %-15s %s\n","-lm", "Read hashes from stdin, upload them to a memcached server (requires the -memcached option).");
printf("  %-15s %s\n","-X", "In CHECK or MATCH mode only examine executable files.");
printf("  %-15s %s\n","-exec", "In CHECK or MATCH mode only examine executable files.");
printf("  %-15s %s\n","-memcached <server>", "Specify memcached server. (Overrides reading list from stdin if used with -m, -c or -cf).");
printf("  %-15s %s\n","-mcd <server>", "Specify memcached server. (Overrides reading list from stdin if used with -m, -c or -cf).");
printf("  %-15s %s\n","-h <script>", "Script to run when a file fails CHECK mode, or is found in MATCH mode.");
printf("  %-15s %s\n","-hook <script>", "Script to run when a file fails CHECK mode, or is found in FIND mode");
printf("  %-15s %s\n","-color", "Use ANSI color codes on output when checking hashes.");
printf("  %-15s %s\n","-strict", "Strict mode: when checking, check file mtime, owner, group, and inode as well as it's hash");
printf("  %-15s %s\n","-S", "Strict mode: when checking, check file mtime, owner, group, and inode as well as it's hash");
printf("  %-15s %s\n","-d","dereference (follow) symlinks"); 
printf("  %-15s %s\n","-fs", "Stay one one file system");
printf("  %-15s %s\n","-dirmode", "DirMode: Read all files in directory and create one hash for them!");
printf("  %-15s %s\n","-devmode", "DevMode: read from a file EVEN OF IT'S A DEVNODE");
printf("  %-15s %s\n","-lines", "Read lines from stdin and hash each line independantly.");
printf("  %-15s %s\n","-rawlines", "Read lines from stdin and hash each line independantly, INCLUDING any trailing whitespace. (This is compatible with 'echo text | md5sum')");
printf("  %-15s %s\n","-rl", "Read lines from stdin and hash each line independantly, INCLUDING any trailing whitespace. (This is compatible with 'echo text | md5sum')");
printf("  %-15s %s\n","-cgi", "Run in HTTP CGI mode");
printf("  %-15s %s\n","-net", "Treat 'file' arguments as either ssh or http URLs, and pull files over the network and then hash them (Allows hashing of files on remote machines).");
printf("  %-15s %s\n","", "URLs are in the format ssh://[username]:[password]@[host]:[port] or http://[username]:[password]@[host]:[port]..");
printf("  %-15s %s\n","-idfile <path>", "Path to an ssh private key file to use to authenticate INSTEAD OF A PASSWORD when pulling files via ssh.");
printf("  %-15s %s\n","-xattr", "Use eXtended file ATTRibutes. In hash mode, store hashes in the file attributes, in check mode compare against hashes stored in file attributes.");
printf("  %-15s %s\n","-u <types>", "Update. In checking mode, update hashes for the files as you go. <types> is a comma-separated list of things to update, which can be 'xattr' 'memcached' or a file name. This will update these targets with the hash that was found at the time of checking.");


/*
else if (strcmp(argv[i],"-xattr")==0) CommandLineHandleArg(argc, argv, i, FLAG_XATTR, "", "",Ctx->Vars);
else if (strcmp(argv[i],"-attrs")==0) 
*/


printf("\n\nHashrat can also detect if it's being run under any of the following names (e.g., via symlinks)\n\n");
printf("  %-15s %s\n","md5sum","run with '-trad -md5'");
printf("  %-15s %s\n","shasum","run with '-trad -sha1'");
printf("  %-15s %s\n","sha1sum","run with '-trad -sha1'");
printf("  %-15s %s\n","sha256sum","run with '-trad -sha256'");
printf("  %-15s %s\n","sha512sum","run with '-trad -sha512'");
printf("  %-15s %s\n","jh244sum","run with '-trad -jh244'");
printf("  %-15s %s\n","jh256sum","run with '-trad -jh256'");
printf("  %-15s %s\n","jh384sum","run with '-trad -jh384'");
printf("  %-15s %s\n","jh512sum","run with '-trad -jh512'");
printf("  %-15s %s\n","whirlpoolsum","run with '-trad -whirl'");
printf("  %-15s %s\n","hashrat.cgi","run in web-enabled 'cgi mode'");
}


