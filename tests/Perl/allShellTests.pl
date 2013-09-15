#!/usr/bin/perl -w

use strict;
use warnings;

use FindBin;                 
use lib "$FindBin::Bin";
use Expect;

use Test::More tests => 4;

my $command= "./ensishell";
my @params= ();
my $timeout=1;
my $patidx=-1;

my $eshell = Expect->spawn($command, @params)
    or die "Echec du démarrage de: $command @params";

# pour voir les commandes, mettre cette variables à 1
$eshell->log_stdout(0);

# variante
$patidx = $eshell->expect($timeout, [ qr/^Variante (\d+): (.*)\r/ ] );
is($patidx, 1, "affichage de la variante");

# prompt
$patidx = $eshell->expect($timeout, [ qr/ensishell>/ ] );
is($patidx, 1, "the prompt is ensishell>");

# fork et exec
$eshell->send("seq -s x 0 3");
$patidx = $eshell->expect($timeout, [ qr/0x1x2x3/ ] );
is($patidx, 1, "fork et exec avec arguments du seq");
$eshell->send("printf toto%dtoto 10");
$patidx = $eshell->expect($timeout, [ qr/toto10toto/ ] );
is($patidx, 1, "fork et exec avec arguments du printf");

# entrées sorties
$eshell->send("ls > totoExpect.txt");
$eshell->send("ls totoExpect.txt");

$eshell->send("rm -f totoExpect.txt titiExpect.txt")


$eshell->send("exit");

ok(1 == 1);
ok(1 == 1);

$eshell->hard_close();


