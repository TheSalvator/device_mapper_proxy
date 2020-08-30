#!/bin/bash

if [ -z "$(lsmod | grep dmp)" ]; then
    if [ -a dmp.ko ]; then
        sudo insmod dmp.ko
    else
        echo "Please run build first"
    fi
else
    echo "Module already loaded"
    exit
fi

if [ -z "$(lsmod | grep dmp)" ]; then
    echo "Something gone wrong"
else
    echo "Success!"
fi
