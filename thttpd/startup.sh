kill -TERM `ps -ef | grep "thttpd ./bench.conf" | grep -v grep | awk '{print $2}'`
sleep 1
./thttpd ./bench.conf
