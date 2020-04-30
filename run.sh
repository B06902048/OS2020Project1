#!bin/bash

for i in 2
do
	sudo dmesg -c
	sudo ./main < ../OS_PJ1_Test/RR_${i}.txt > output/RR_${i}_stdout.txt
	sudo dmesg | grep Project1 > temp.txt
	python3 ../cal.py < temp.txt > output/RR_${i}_dmesg.txt
done

rm -rf temp.txt
