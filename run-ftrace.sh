cur_dir=$pwd

sleep_time=3
./test-malloc $sleep_time &
#PID=$(ps -aux | grep "test-malloc" | grep -v "grep" | awk '{print $2}')
PID=$(pidof test-malloc)
echo "Process ID is: $PID"

cd /sys/kernel/tracing
echo > trace #Empty trace buffer

echo function_graph > current_tracer
#echo function > current_tracer
#echo __x64_sys_write > set_ftrace_filter
echo > set_ftrace_filter

echo $PID > set_ftrace_pid
echo 1 > tracing_on

sleep $sleep_time

#cat trace | head -20 > $cur_dir/ftrace.dump
echo $cur_dir

echo 0 > tracing_on

sleep 1 # Wait to flush the trace buffer

cat trace
