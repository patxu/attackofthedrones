#!/bin/bash

function test {
    "$@"
    local status=$?
    if [ $status -ne 0 ]; then
        echo "ERROR: with $1" >&2
        echo -e "\nexiting\n"
        exit 1
    fi
    return $status
}

cd ./bin
echo -e "\nRunning cmake..."
test cmake ..

echo -e "\nRunning make..."
test make

echo -e "\nSuccess!"

echo -e "\npress <enter> to run bin/control\n"
read input
./control
