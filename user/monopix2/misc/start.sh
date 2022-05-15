BINPATH=/mnt/data/monopix2/eudaq2/bin
IP_RUNCTRL=192.168.130.74 

$BINPATH/euRun &
sleep 1
$BINPATH/euLog -r $IP_RUNCTRL &
sleep 1
$BINPATH/euCliCollector -n DirectSaveDataCollector -t ds_dc -r $IP_RUNCTRL &
$BINPATH/euCliProducer -n Monopix2Producer -t monopix2 -r $IP_RUNCTRL &
