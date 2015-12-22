kill -TERM `ps -ef | grep "gamesvr_mark ./bench.conf" | grep -v grep | awk '{print $2}'`
sleep 1
./gamesvr_mark ./bench.conf
