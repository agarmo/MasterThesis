# '--*-Perl-*--';
# linkmex.pl
# part of gnumex (version 2)
# original authors Matthew Brett and Tom Van Overbeek
# last changed by Kristjan Jonasson (jonasson@hi.is) Dec. 2007
#
# perl script to link mex file dll / engine file exe
# create library files if not using precompiled libraries
# create temporary files for dll creation
# create dll / exe
# delete temporary files


$mtype      = $ENV{'GM_MEXTYPE'};   # mex or engine
$lang       = $ENV{'GM_MEXLANG'};   # c or Fortran
$dllcmd     = $ENV{'GM_DLLTOOL'};   # dlltool command
$addlibs    = $ENV{'GM_ADD_LIBS'};  # added libraries
$mexdef     = $ENV{'GM_MEXDEF'};    # mex.def or fmex.def
$libpath    = $ENV{'GM_QLIB_NAME'}; # location of libraries and mex.def
$defpath    = $ENV{'GM_DEF_PATH'};  # location of other .def files
$compiler   = $ENV{'COMPILER'};     # gcc, g77, g95 or gfortran

foreach (@ARGV) {
  $i=index($_," ");
  if ($i >= 0) { $_ = '"' . $_ . '"'; }
}
$arglist = join(" ", @ARGV); # -o xxx.mexw32 -s file1.obj file2.obj...
if ($lang eq 'c') {
  # need g++ if this is c++
  # unfortunately you cannot directly set a different linker
  # for c++ from the options file, so we'll use the GM_ISCPP
  # word passed to the linker as an indicator
  # (and remove it)
  $iscppf = $arglist =~ s/GM_ISCPP//;
  if ($mtype eq "mex") {
    if ($iscppf) {
      $linker = 'g++ -shared';
    } else {
      $linker = 'gcc -shared';
    }
  } else { # engine
    $linker = "g++";
  }
} elsif ($lang eq 'f' || $lang eq 'f77') { # fortran 77
  if ($mtype eq "mex") {
    $linker = 'g77 -shared';
  } else {
    $linker = "g77";
  }
} else { # Fortran 95 (g95 or gfortran)
  if ($mtype eq "mex") {
    $linker = $compiler . ' -shared';
  } else {
    $linker = $compiler;
  }
} 

if ($mtype eq 'mex') { # command to make mex dll
  $cmd = join(" ", $linker, $mexdef, $arglist, $addlibs);
} else { # engine file
  $cmd = join(" ", $linker, $arglist, '-lkernel32','-luser32','-lgdi32',$addlibs,'-mwindows');
}

# print command (will only be printed if -v switch is given with mex):
#print "INFORMATION FROM LINKMEX.PL:\n";
#print "windows path  = " . $ENV{'PATH'} . "\n";
print "link command: " . $cmd . "\n";
# execute via backticks (system doesn't work on W9x)
$message = `$cmd`;
