# smlock
Smlock is a minimal screen locker, for Xorg, optimized for mobile.  
Fork of Benruijl sflock - github.com/benruijl/sflock  
It shows a clock, that will update at key press.  
## Install
REQUIRES xlib.  
On Arch:  
`sudo pacman -S base-devel xlib11`  
On Debian, Ubuntu:  
`sudo apt install build-essential libxtst-dev`  
Build and install:  
`sudo make clean install`  
## Options
\-c   modify the characters displayed when the user enters his password. This can be a sequence of characters to create a fake password  
\-f   the name/messagge font  
\-g   the clock font  
\-b   display a custom message instead of the user name  
\-i   a command to be executed at start  
\-e   a command to be executed at the end  
\-h   hide user name or messagge  
\-?   help  
Example:  
`smlock -i onboard -h`  
