#!/bin/bash

CLIENT=./kvclient
HOST=127.0.0.1
PORT=4444

NUM_CLIENTS=16383

for i in $(seq 1 $NUM_CLIENTS); do
(
    {
        echo "set key$i value$i ;"
        echo "get key$i ;"
        echo "del key$i ;"
        echo "exit ;"
    } | $CLIENT $HOST $PORT
) &
done

wait

echo "Finished."