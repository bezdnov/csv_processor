#!/bin/bash

i=1

pwd

for filename in ./tests/*; do
    echo "Test number $i"
    echo "Input file: $filename"
    cat $filename
    echo "Program output:"
    ./csvreader "$filename"
    echo "-------------------------------"
    ((i++))
done
