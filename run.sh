#!/bin/bash

sudo dmesg -c

sudo ./main < ../OS_PJ1_Test/TIME_MEASUREMENT.txt > ./output/TIME_MEASUREMENT_stdout.txt
sudo dmesg | grep Project1 > ./temp.txt
sudo python3 ../cal.py < temp.txt > ./output/TIME_MEASUREMENT_dmesg.txt
sudo dmesg -c
rm temp.txt

for i in 1 2 3 4 5
do
	sudo ./main < ../OS_PJ1_Test/FIFO_$i.txt > ./output/FIFO_${i}_stdout.txt
	sudo dmesg | grep Project1 > ./temp.txt
	sudo python3 ../cal.py < temp.txt > ./output/FIFO_${i}_dmesg.txt
	sudo dmesg -c
	rm temp.txt
done	

for i in 1 2 3 4 5
do
	sudo ./main < ../OS_PJ1_Test/RR_$i.txt > ./output/RR_${i}_stdout.txt
	sudo dmesg | grep Project1 > ./temp.txt
	sudo python3 ../cal.py < temp.txt > ./output/RR_${i}_dmesg.txt
	sudo dmesg -c
	rm temp.txt
done	

for i in 1 2 3 4 5
do
	sudo ./main < ../OS_PJ1_Test/SJF_$i.txt > ./output/SJF_${i}_stdout.txt
	sudo dmesg | grep Project1 > ./temp.txt
	sudo python3 ../cal.py < temp.txt > ./output/SJF_${i}_dmesg.txt
	sudo dmesg -c
	rm temp.txt
done	


for i in 1 2 3 4 5
do
	sudo ./main < ../OS_PJ1_Test/PSJF_$i.txt > ./output/PSJF_${i}_stdout.txt
	sudo dmesg | grep Project1 > ./temp.txt
	sudo python3 ../cal.py < temp.txt > ./output/PSJF_${i}_dmesg.txt
	sudo dmesg -c
	rm temp.txt
done	



