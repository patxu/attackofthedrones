#!/bin/bash

cd ./bin
#rm -rf *
#cmake ..
make
cd ../bin/
echo -e "\npress <enter> to continue\n"
read input
./control
