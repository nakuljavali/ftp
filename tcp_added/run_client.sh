#!/bin/bash

rm -f /tmp/output.bin

start=`date +%s`


./client $1 $2 
./server $2

end=`date +%s`


runtime=$((end-start))

echo "Run time is $runtime "
md5sum /tmp/batch.bin
