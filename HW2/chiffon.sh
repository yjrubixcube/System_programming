while getopts l:m:n: flag
do
    case "${flag}" in
        m) n_hosts=${OPTARG};;
        n) n_players=${OPTARG};;
        l) lucky_number=${OPTARG};;
    esac
done
#echo "ln is $lucky_number";
#echo "np is $n_players"
#echo "nh is $n_hosts"
fifo_list=("fifo_0.tmp" "fifo_1.tmp" "fifo_2.tmp" "fifo_3.tmp" "fifo_4.tmp" "fifo_5.tmp" "fifo_6.tmp" "fifo_7.tmp" "fifo_8.tmp" "fifo_9.tmp" "fifo_10.tmp")
i=0
j=3
rm fifo_*.tmp -f
while [[ $i -le $n_hosts ]]; do
    #echo "making $i"
    mkfifo ${fifo_list[$i]}
    exec {j}<> ${fifo_list[$i]}
    i=$(($i+1))
    j=$(($j+1))
done
i=0
while [[ $i -le $n_hosts ]]; do
    #echo "writing $i"
    if [[ $i -ge 1 ]]; then
        ./host -l $lucky_number -m $i -d 0 &
    fi
    i=$(($i+1))
done
count=1
# 13 in total
tally=(0 0 0 0 0 0 0 0 0 0 0 0 0)
#todo=0
#tally=(0 10 20 40 40 60 60 60 80 90 100 110 120)
for (( x_1 = 1 ; x_1 <= ${n_players} ; x_1++)); do
    for (( x_2 = x_1+1 ; x_2<= ${n_players} ; x_2++)); do
        for (( x_3 = x_2+1 ; x_3<= ${n_players} ; x_3++)); do
            for (( x_4 = x_3+1 ; x_4<= ${n_players} ; x_4++)); do
                for (( x_5 = x_4+1 ; x_5<= ${n_players} ; x_5++)); do
                    for (( x_6 = x_5+1 ; x_6<= ${n_players} ; x_6++)); do
                        for (( x_7 = x_6+1 ; x_7<= ${n_players} ; x_7++)); do
                            for (( x_8 = x_7+1 ; x_8<= ${n_players} ; x_8++)); do
                                if [[ $count -le ${n_hosts} ]]; then
                                    echo "$x_1 $x_2 $x_3 $x_4 $x_5 $x_6 $x_7 $x_8" > ${fifo_list[$count]}
                                    #echo "sending $x_1 $x_2 $x_3 $x_4 $x_5 $x_6 $x_7 $x_8 to $count"
                                    count=$(($count+1))
                                    #todo=$(($todo+1))
                                else
                                    for (( round = 0 ; round < 9 ; round++ )); do
                                        if [[ round -eq 0 ]]; then
                                            read line < "fifo_0.tmp"
                                            host=$line
                                            #echo "reading id = $host"
                                        else
                                            read -a line < "fifo_0.tmp"
                                            tally[${line[0]}]=$((${tally[${line[0]}]}+${line[1]}))
                                        fi
                                        if [[ round -eq 8 ]]; then
                                            echo "$x_1 $x_2 $x_3 $x_4 $x_5 $x_6 $x_7 $x_8" > ${fifo_list[$host]}
                                            #echo "sending $x_1 $x_2 $x_3 $x_4 $x_5 $x_6 $x_7 $x_8 to $host"
                                        fi
                                    done
                                fi
                            done
                        done
                    done
                done
            done
        done
    done
done
#readall
#echo "out of loop"
#count=$(($count-1))
while [ $count -gt 1 ];do
    read line < "fifo_0.tmp"
    host=$line
    #echo "host is $host"
    for (( j = 0 ; j < 8 ; j++ )); do
        read -a line < "fifo_0.tmp"
        #echo "0= ${line[0]}, 1=${line[1]}"
        tally[${line[0]}]=$((${tally[${line[0]}]}+${line[1]}))
    done
    count=$(($count-1))
done
#echo "done readig"
for (( count = 1 ; count <= ${n_hosts} ; count++ )); do
    echo "-1 -1 -1 -1 -1 -1 -1 -1" > ${fifo_list[$count]}
done

#sort result
sorted=(0)
for (( i = 1 ; i <= ${n_players} ; i++ )); do
    sorted=(${sorted[@]} ${tally[$i]})
    #echo "$i is ${sorted[$i]}"
done
#echo "${sorted[@]}"
ranking=(0 0 0 0 0 0 0 0 0 0 0 0 0)
#rank=1
for (( rank = 1 ; rank <= ${n_players} ; )); do
    count=0
    max=-1
    for (( i = 1 ; i <= ${n_players} ; i++ )); do
        if [[ ${sorted[$i]} -gt max ]]; then
            max=${sorted[$i]}
        fi
    done

    for (( i = 1 ; i <= ${n_players} ; i++ )); do
        if [[ ${sorted[$i]} -eq max ]]; then
            ranking[$i]=$rank
            sorted[$i]=$((-1))
            count=$(($count+1))
        fi
    done
    rank=$(($rank+$count))
done
#echo "${ranking[@]}"

for (( i = 1 ; i <= ${n_players} ; i++ )); do
    echo "$i ${ranking[$i]}"
done
rm fifo_*.tmp -f
wait
