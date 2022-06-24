#"source" me to start all required EUDAQ components

#adjust paths to local machine
MONITOR_BIN=/mnt/data/monopix2/rd50-mpw3-gui/build/
EUDAQ_BINS=/mnt/data/monopix2/eudaq2/bin
BDAQ_SCANS=/mnt/data/monopix2/tj-monopix2-daq/tjmonopix2/scans/
HAMEG_PRODUCER=/home/silicon/.TJ-Monopix2-Power/hameg_producer.py
IP_RUNCTRL=192.168.130.140

#needed by producer to  find eudaq pybind library
export PYTHONPATH="/mnt/data/monopix2/eudaq2/lib" # ""$PYTHONPATH:/mnt/data/monopix2/eudaq2/lib"

#$EUDAQ_BINS/euRun &
#sleep 1
#$EUDAQ_BINS/euLog -r $IP_RUNCTRL &
#sleep 1
$EUDAQ_BINS/euCliCollector -n DirectSaveDataCollector -t ds_dc -r $IP_RUNCTRL &
python $BDAQ_SCANS/monopix2_producer.py -r $IP_RUNCTRL&
$MONITOR_BIN/MPW3_gui -c $MONITOR_BIN/monopix2_gui.config & 
python $HAMEG_PRODUCER -r $IP_RUNCTRL &
