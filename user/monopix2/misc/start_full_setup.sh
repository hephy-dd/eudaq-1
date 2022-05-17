#"source" me to start all required EUDAQ components

#adjust paths to local machine
EUDAQ_BINS=/mnt/data/monopix2/eudaq2/bin
BDAQ_SCANS=/mnt/data/monopix2/tj-monopix2-daq/tjmonopix2/scans/
IP_RUNCTRL=192.168.130.74 

#needed by producer to  find eudaq pybind library
export PYTHONPATH="/mnt/data/monopix2/eudaq2/lib" # ""$PYTHONPATH:/mnt/data/monopix2/eudaq2/lib"

$EUDAQ_BINS/euRun &
sleep 1
$EUDAQ_BINS/euLog -r $IP_RUNCTRL &
sleep 1
$EUDAQ_BINS/euCliCollector -n DirectSaveDataCollector -t ds_dc -r $IP_RUNCTRL &
#$EUDAQ_BINS/euCliProducer -n Monopix2Producer -t monopix2 -r $IP_RUNCTRL &
python $BDAQ_SCANS/monopix2_producer.py
