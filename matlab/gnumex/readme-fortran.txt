SOME COMMENTS ON FORTRAN MEX FILES

INTRODUCTION
  This document extends the information in the accompanying file README. Gnumex
  version 2.00 supports both Fortran 77 (with the g77 compiler) and Fortran
  90/95 (with either the g95 or gfortran compiler). The following guidelines
  may help clear things regarding Fortran programs.

FORTRAN FILE EXTENSIONS
  Fortran 77 programs and Fortran 90/95 programs in fixed source form should
  have extension .f, but free form Fortran 95 programs must have extension .f90
  (the compilers support also the extension .f95, but Matlab's mex command does
  not, and .f90 is generally preferred over .f95 anyway).

CASE OPTIONS AND TRAILING UNDERSCORES
  With g77 and g95 Gnumex uses the compile switches -fcase-upper and
  -fno-underscoring (which both g77 and g95 support). The first means that
  exported symbols are translated to all-upper-case in the created .obj files
  (the case used in the source file is thus irrelevant), and the second means
  that trailing underscores are not added to these symbols (which is otherwise
  the default). The Fortran mx and mex-functions of Matlab have all-uppercase
  names (despite the Matlab documentation and demos using mixed-case for them)
  and do not have trailing underscores. If Fortran programs are called from C
  they must therefore be called with all-uppercase. It would be possible to
  change to using the switch -fcase-preserve, and then C and Fortran could be
  mixed more freely (e.g. by calling C-functions that are not all-upper-case
  from Fortran), but then the gateway routine and all mx and mex functions used
  must be spelled with all-upper-case (MEXFUNCTION and MXGETPR for example). As
  -fcase-lower folds everything to lower case it is not very useful in this
  context.

  In contrast gfortran does not offer the -fcase switches, and in fact defaults
  to the behaviour of -fcase-lower. To circumvent the problem that this causes
  one can use BIND(C,NAME...) clauses in interface blocks for the mx and mex
  functions, e.g. BIND(C,NAME='MXGETPR') (the mx and mex functions are probably
  written in C anyway). The demonstration programs give more details.

MEX FUNCTIONS WITH CHARACTER STRING ARGUMENTS
  The Fortran functions mexErrMsgTxt and mexPrintf (really MEXERRMSGTXT and
  MEXPRINTF), that both have a single character argument, seem to work fine with
  both g77 and g95 (with the interface in mexinterface.f90 discussed below).
  However, gfortran seems to have a different mechanism for passing character
  strings, and the Fortran versions of these functions do not work. Thus
  mexinterface_c.f90 binds them to the corresponding C-functions (with mixed
  case names). This meens that strings passed to them from gfortran must have
  an ascii zero appended, viz:
       call mexprintf('A message'//char(0))
       call mexerrmsgtxt('An error occurred'//char(10)//char(0))

POINTERS AND %VAL
  Matlab sends integer memory addresses (pointers) to the gateway routine and
  these must be translated to arrays. In g77 the construct %val may be used, but
  in g95 it is possible to use native pointers. Again the demonstration programs
  give the details.

CYGWIN
  Cygwin with -mno-cygwin switch (which is more or less equivalent to using 
  mingw) currently (2007/8) works with g77 and g95, and can be made to work with
  gfortran with some hacking (see next section). Cygwin (with gcc 3.2) with its
  own libraries, however seems to work only with C and g77.

GFORTRAN WITH -MNO-CYGWIN
  To allow "Cygwin -mno-cygwin" linking to work with gfortran 4.3, the
  mexopts.bat file must be edited to contain the lines:
    set LIBRARY_PATH=/lib/mingw:/usr/local/gfortran/lib/gcc/i686-pc-cygwin/4.3.0
    set LIBRARY_PATH=/usr/local/gfortran/lib:/lib:%LIBRARY_PATH%
  In addition one must copy the file dllcrt2.o from /lib/mingw to the current
  directory. For other versions of gfortran suitable ammendments must be made.

DEMONSTRATION PROGRAMS
  The demo Fortran programs that come with Matlab some use the %val construct
  but others use instead the functions mxCopyPtrToXXX and mxCopyXXXToPtr. With
  g77 it is much simpler to stick with the %val construct. The example gateway
  routine of Matlab, yprimefg.f (in <matlabroot>\extern\examples\mex) is
  totally incompatible with all the gnu Fotran compilers. However with Gnumex
  there are routines, yprime77.f and yprime95.f90 which are compatible. The
  first one calls the computational routine in yprimef.f, but the second
  contains a module with a computational routine that is called (the yprimef.f
  that comes with Matlab is actually g77-compatible, but for convenience a
  similar routine is enclosed with Gnumex). The demonstartion programs are
  located in a subfolder, examples, in the folder where Gnumex is installed.

  For Fortran 90 there is an additional demonstration program, powerit.f90,
  which carries out power iteration (to find dominant eigenvalue) with a sparse
  matrix. The Fortran 90 demo programs include two modules, mexinterface_c.f90
  and mexinterface.f90. The second one is simpler but does not work with
  gfortran (because of the -fcase issue discussed above), but the second one
  works for both compilers. These modules define the interface to those mx-
  and mex-functions which are used by the demo programs, and if other functions
  are needed their interface must be added here.

RUNNING THE DEMOS
  To try out Fortran mexing use Gnumex to create an options.bat file for
  Fortran, compile with mex and call yprime77 or yprime95. From the Matlab
  prompt one can for instance issue:
      >> gnumex fortran77
      >> mex yprime77.f yprimef.f
      >> yprime77(1, 1:4)
  or:
      >> gnumex g95
      >> mex -c mexinterface.f90
      >> mex yprime95.f90
      >> yprime95(1, 1:4)
  or:
      >> gnumex gfortran
      >> mex -c mexinterface_c.f90
      >> mex yprime95.f90
      >> yprime95(1, 1:4)
  and should in each case receive the answer: 2.0000  8.9685  4.0000  -1.0947
  (here it has been assumed that the Gnumex folder is on Matlab's search path).
  The compilations of the mexinterface files must come before compiling yprime95
  so that the module files (.mod) are created timely. With g95 either 
  mexinterface module may be used.

  To try powerit, issue:
      >> mex powerit.f90 mexinterface_c.obj  [or mexinterface.obj for g95]
      >> n = 1024
      >> a = gallery('neumann', n);
      >> x = [1; zeros(n-1,1)];
      >> powerit(a,x,1000)
  The answer 7.9972 should be received. Powerit can also be called with 5
  arguments; see comments in powerit.f90 for details.

VERSIONS OF THE COMPILERS
  Gnumex 2.0 is tested with the following versions of the compilers and Matlab:

  MINGW
    g77: 3.4.2 (MinGW 5.1.3, sourceforge.net/project/showfiles.php?group_id=2435)
    g95: 0.91 (with gcc 4.1.2, ftp.g95.org/g95-MinGW-41.exe)
    gfortran: 4.3.0 20071130 (quatramaran.ens.fr/~coudert/gfortran)

  CYGWIN-MNO-CYGWIN
    g77: 3.4.4 (www.cygwin.com)
    g95: 0.90 (with gcc 4.0.3, ftp.g95.org/g95-Cygwin-41.exe)
    gfortran: 4.3.0 20070805 (quatramaran.ens.fr/~coudert/gfortran)

  CYGWIN-3.2
    g77: 3.2 (ptolemy.eecs.berkeley.edu/ptolemyII/ptII4.0/cygwin.htm)

  MATLAB
    Versions 7.1 through 7.6.

(KJ, April. 2008)
