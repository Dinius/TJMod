#!/usr/bin/perl -w
use strict;
use Tk;
use File::Copy;
use Win32::Process;


our $ipaddr;
our $srcdir;
our $gamedir;
our @files;
our $config;
our $config2;
our $serverconfig;

my $settingsfile = "./tjmod_dev_settings.pl";
require "$settingsfile";


# GUI - I can't get by without GUIs..! :P
my $mw = MainWindow->new(-title=>"DevTool - TJMod");
$mw ->resizable( 0, 0 );
$mw ->geometry("225x300");

my $t = $mw->Frame()->pack(-anchor=>"n", -side=>"top", -fill=>"x");
my $m = $mw->Frame()->pack(-anchor=>"n", -side=>"top", -fill=>"x");

my $button_t0 = $t
    ->Button(-text=>"SRC", -padx=>20, -pady=>5, -justify=>"left", -relief=>"flat", -command=>\&omod)
    ->pack(-side=>"left", -fill=>"x", -expand=>1)
    ;
my $button_t1 = $t
    ->Button(-text=>"MOD", -padx=>20, -pady=>5, -justify=>"left", -relief=>"flat", -command=>\&ogame)
    ->pack(-side=>"left", -fill=>"x", -expand=>1)
    ;
    
my $button_m0 = $m
    ->Button(-text=>"Copy", -padx=>50, -pady=>15, -relief=>"ridge", -command=>\&copyfiles)
    ->pack(-fill=>"x", -expand=>1)
    ;

my $button_m1 = $m
    ->Button(-text=>"Server", -padx=>50, -pady=>15, -relief=>"ridge", -command=>\&server)
    ->pack(-fill=>"x", -expand=>1)
    ;

my $button_m2 = $m
    ->Button(-text=>"Player", -padx=>50, -pady=>15, -relief=>"ridge", -command=>\&player)
    ->pack(-fill=>"x", -expand=>1)
    ;
    
my $button_m2_2 = $m
    ->Button(-text=>"MultiPlayer", -padx=>50, -pady=>15, -relief=>"ridge", -command=>\&player2)
    ->pack(-fill=>"x", -expand=>1)
    ;

my $button_m3 = $m
    ->Button(-text=>"Go Play", -padx=>50, -pady=>30, -relief=>"ridge", -command=>\&goplay)
    ->pack(-fill=>"x", -expand=>1)
    ;
    
MainLoop();

sub omod  {system('%windir%\explorer '. $srcdir);}
sub ogame {system('%windir%\explorer '. $gamedir);}

sub copyfiles
{
    foreach my $file (@files)
    {
        copy("$srcdir/$file", "$gamedir/tjmod/$file")
    }
}

my $serverexe;
sub server
{  
    if (defined($serverexe)) {$serverexe->Kill(1);}
    
    &copyfiles();
    
    Win32::Process::Create($serverexe,
        "$gamedir/ETDED.exe",
        "+set sv_pure 0 +set fs_game tjmod +dedicated 1 +exec $serverconfig",
        0,  NORMAL_PRIORITY_CLASS,
        "$gamedir")|| die errorreport()
}

my $playerexe;
sub player
{    
    if (defined($playerexe)) {$playerexe->Kill(1);}
    
    Win32::Process::Create($playerexe,
        "$gamedir/ET.exe",
        "+set sv_pure 0 +set fs_game tjmod +exec $config +connect $ipaddr",
        0,  NORMAL_PRIORITY_CLASS,
        "$gamedir")|| die errorreport()
}

my $playerexe2;
sub player2
{    
    Win32::Process::Create($playerexe2,
        "$gamedir/ET.exe",
        "+set sv_pure 0 +set fs_game tjmod +exec $config +connect $ipaddr +exec $config2",
        0,  NORMAL_PRIORITY_CLASS,
        "$gamedir")|| die errorreport()
}

# Copy files -> wait -> Start Server -> wait -> Start Player
sub goplay{&copyfiles();sleep 1;&server();sleep 1;&player();}