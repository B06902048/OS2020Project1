#!/bin/bash

sudo dmesg -c

sudo ./main < ../OS_PJ1_Test/TIME_MEASUREMENT.txt > ./output/TIME_MEASUREMENT_stdout.txt
sudo dmesg | grep Project1 > ./temp.txt
sudo python3 ../cal.py < temp.txt > ./output/TIME_MEASUREMENT_dmesg.txt
sudo dmesg -c
rm temp.txt

