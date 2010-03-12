$infile     = @ARGV[0];
$header_file = "template/header.html";
$footer_file = "template/footer.html";

$inf = "";
open(FHF, $infile) or die "Error: $infile not found!\n";
while ( <FHF> ) {
    $inf = $inf . $_;
}
close(FHF);

$footer = "";
open(FHF, $footer_file) or die "Error: $footer not found!\n";
while ( <FHF> ) {
    $footer = $footer . $_;
}
close(FHF);

$header = "";
open(FHF, $header_file) or die "Error: $header not found!\n";
while ( <FHF> ) {
    $header = $header . $_;
}
close(FHF);



$inf =~ s/\<!--AUTO_INSERT_FOOTER--\>/$footer/ig;
$inf =~ s/\<!--AUTO_INSERT_HEADER--\>/$header/ig;

print $inf

