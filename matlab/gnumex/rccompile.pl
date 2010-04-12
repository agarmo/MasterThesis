# '--*-Perl-*--';
# rccompile.pl
# 
# pass options etc to windres.  Note that a pretend .res file
# extension for output .o file has been passed, to fool mex.bat.
# We need unix paths for windres, if --unix option has been passed
# (needed as Cygwin windres does not handle DOS paths)
# Also necessary to remove -D, replace with --define

# flag to convert to unix filenames
$swapf = 0;

# break at "" blocks, and spaces
$str = join(' ',@ARGV);
@g = ($str =~ /\"([^\"]*)\"|(\S+)(?:\s|$)+/g);
foreach $g(@g){if (defined($g)){push(@tokens, $g);}}

# parse for options, filenames
while (@tokens) {
    $t = shift(@tokens);
    if (($opt, $cont) = ($t =~ /^-(\S)(\S*)/)){
	if ($opt =~ /D/){	#-D option
	    if ($cont eq ""){$cont = shift(@tokens)}
	    push(@args, ('--define',$cont));
	} elsif ($opt =~ /o/) { #-o fname option
	    if ($cont eq ""){$cont = shift(@tokens)}
	    push(@args, ('-o',$cont));
	    push(@fileargs, $#args);
	} elsif ($opt =~ /-/ && ($cont ne "")) { #-- options
	    $opt = $cont;
	    if ($opt =~ /^unix/) {$swapf=1}; #--unix option
	}
    } else {			#not a - as first character
	# must be a filename
	push(@args, $t);
	push(@fileargs, $#args);
    }
}

# do unix filename conversion if --unix option passed
foreach $fagno(@fileargs) {
    if ($swapf) {
	$args[$fagno]=`cygpath -u \"$args[$fagno]\"`;
	chop($args[$fagno]);
	$args[$fagno]=~ s/\ /\\\ /g;
    } else { # not unix, need "" to protect spaces
	$args[$fagno]="\"$args[$fagno]\"";
    }
}

# execute via backticks (system doesn't work on W9x)
$cmd = join(' ',('windres', @args, " -O coff"));
$message = `$cmd`;
print $message unless ($message eq "");
