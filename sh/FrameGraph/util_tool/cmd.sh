# $1: program name
# you should run this sh in data folder

# run your program first

pid=$(pidof $1)

if [ -z "$pid" ]
then
    echo "can't find pid of $1"
    exit
else
    echo -e "pid of $1 is: $pid"
fi

sampling_rate=10000
echo "sampling rate is: $sampling_rate (Hz)"

monitor_time=30
echo "monitor time is: $monitor_time (s)"

time=$(date +"%T")
echo "start at: $time"

sudo perf record -F $sampling_rate -p $pid -g -o in-fb.data -- sleep $monitor_time
sudo perf script -i in-fb.data > in-fb.perf

/opt/FlameGraph/stackcollapse-perf.pl in-fb.perf > in-fb.folded
/opt/FlameGraph/flamegraph.pl in-fb.folded > "$1_$time.svg"

# remove
rm -f in-fb.data
rm in-fb.perf
rm in-fb.folded
