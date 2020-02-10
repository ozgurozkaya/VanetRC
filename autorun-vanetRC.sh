#!/bin/sh
# This is a comment!
cd ../../
for i in $(seq 1 1 10)
do
    #run VanetRC with different seed value (i)
    echo "\t------ Seed Number: "$i" -------"
    ./waf --run "VanetRC --seed="+$i+" --connections=5"
done