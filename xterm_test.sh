#!/bin/bash

export FILE=/tmp/chat_$2
echo "join $1 $2" >$FILE
cat $P/commands >>$FILE
netcat localhost 1234 <$FILE
