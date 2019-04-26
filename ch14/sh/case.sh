#!/bin/bash

case "$1" in
    'sys')
        uname -a
        ;;
    'cpu')
        lscpu
        ;;
    'time')
        date
        ;;
    'ps')
        ps -e
        ;;
esac
