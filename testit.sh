#!/bin/bash
export XTERM=xterm
export HOST=localhost
export PORT=1234

export P=`pwd`

killall server 2>/dev/null
killall $XTERM 2>/dev/null

daemon -i -- xterm -e $P/server

#daemon -i -X $XTERM -e "/bin/bash $P/server"
daemon -i -- xterm -e "$P/xterm_test.sh room1 bob" 
daemon -i -- xterm -e "$P/xterm_test.sh room1 steve" 
daemon -i -- xterm -e "$P/xterm_test.sh room2 dan"
daemon -i -- xterm -e "$P/xterm_test.sh room2 jeff"
daemon -i -- xterm -e "$P/xterm_test.sh room3 alone"


