#!/bin/bash

funca() {
    for a in $@ ; do
        echo $a
    done
}

funca 1 2 3

for a in $@ ; do
    echo $a
done

