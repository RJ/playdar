#!/usr/bin/perl
# pipe this to your script, eg. ./dummy.pl | myscript

$art = "u2";
$trk = "vertigo";
my $rq = "{ \"qid\":\"4e63397e-18a3-11de-ba95-9bb64e34b615\", \"artist\" : \"$art\", \"track\" : \"$trk\" }";
print pack('N', length($rq));
print $rq;
