#!/bin/bash

#所有参数
echo $@

for a in $@ ; do
    echo $a
done

#参数个数
echo $#

