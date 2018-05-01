
# IP address the client connects to
our $ipaddr = "6.6.6.110";

# You gotta use \ slashes on windows, / willnt work that guud
# Dir of tjmod source
our $srcdir = 'E:\SVN\TJMod\trunk\src\Release';
# Dir of game
our $gamedir = 'D:\WolfDev';

# Config for normal loading
our $config = "Dinius";
# Secondary config for "multiplayer", e.g. containing r_mode 6 and vid_restart.
our $config2 = "small";
# Server config, should contain a map command
our $serverconfig = "server";

# Files to copy from source dir to game dir\tjmod
our @files = (
    "cgame_mp_x86.dll",
    "qagame_mp_x86.dll",
    "ui_mp_x86.dll"
);


1;