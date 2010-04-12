/* mexgcc.c - shell to pass params to gcc, and
renames .o file to .obj.  Assumes last file is .c 
etc file, as is the case for compiles passed by mex.bat.
To recompile type: "gcc -mno-cygwin -o mexgcc.exe mexgcc.c" 
at the bash prompt). */

#include <process.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

int main( int argc, char *argv[ ])
{
   char path_buffer[_MAX_PATH];
   char outputn[_MAX_PATH];
   char drive[_MAX_DRIVE];
   char dir[_MAX_DIR];
   char fname[_MAX_FNAME];
   char ext[_MAX_EXT];
   char** argvcp;
   int i, objf=0, okf;

   if (argc > 1) {
	char prog[] = "gcc";
	argv[0] = prog;
	
	/* copy args, with inverted commas!
	inverted commas are to allow path names with spaces
	to be passed from an MSDOS shell to gcc */
	argvcp=calloc(argc+2, sizeof(char *));
	argvcp[0] = argv[0];
	/* also check here for -o option, add if necessary */
	for (i=1;i<argc;i++){
	  if (argv[i][0] == '-' & argv[i][1] == 'o')
	    objf = 1;
	  argvcp[i]=calloc(strlen(argv[i])+3, sizeof(char));
	  sprintf(argvcp[i], "\"%s\"", argv[i]);
	}

	/* code assumes last file passed was .c file, which 
	   is always true in mex compiles */
	if (!objf) { /* no -o option, we'll have to make one */
	  _splitpath(argv[i-1], drive, dir, fname, ext);
	  _makepath( path_buffer, 0, 0, fname, "obj");
	  sprintf(outputn, "-o \"%s\"", path_buffer);
	  argvcp[i++]=outputn;
	}
	argvcp[i] = NULL;

	/* do gcc */
	okf=spawnvp(_P_WAIT, prog, (const char **)argvcp);
	  
	/* free arg copy */
	for (i=0;i<argc;i++) free(argvcp[i]); free(argvcp);
	
	return(okf);

   } else { // No args passed - display  help message
	printf("mexgcc - passes args to gcc and renames .o file to .obj\n");
	return(1);
   }
}
