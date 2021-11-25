./euRun &
sleep 1
./euLog & 
sleep 1
./euCliCollector -n Ex0TgDataCollector -t my_dc &
./euCliProducer -n Ex0Producer -t my_pd0 &
sleep 1
#./MPW3_gui &
