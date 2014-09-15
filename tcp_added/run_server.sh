#!/bin/bash

rm -f /tmp/output.bin
start=`date +%s`


./server $1

./client /tmp/batch.bin $1

end=`date +%s`


runtime=$((end-start))

echo "Run time is $runtime "
md5sum /tmp/batch.bin