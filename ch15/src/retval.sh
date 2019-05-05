#!/bin/bash

if ./bin/outr -r abcdef ; then
    echo '[OK]'
else
    echo '[NOT OK]'
fi

echo ''

if ./bin/outr -r 12345 -r ; then
    echo '[OK]'
else
    echo '[NOT OK]'
fi

