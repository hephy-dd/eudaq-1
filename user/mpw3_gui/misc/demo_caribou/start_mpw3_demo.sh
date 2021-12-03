./euRun &
sleep 1
./euLog -r 192.168.130.74 & 
sleep 1
./MPW3_gui &
./euCliCollector -n DirectSaveDataCollector -t ds_dc -r 192.168.130.74 &
sleep 2
ssh root@192.168.130.7 "cd /home/root/daq_run; euCliProducer -n CaribouProducer -t RD50_MPW3 -r 192.168.130.74" &
#ssh 'pct-eudaq@192.168.130.140' "cd eudaq/bin; ./euCliProducer -n AidaTluProducer -t aida_tlu -r 192.168.130.74" &
