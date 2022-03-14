Modules for the RD50_MPW3 chip, in particular the "fast-data-path".

## Building

By default the cmake-flag is deactivated '''USER_MPW3_BUILD'''.
When you activate it with the cmake parameter '''-D USER_MPW3_BUILD=ON''' several modules will be built:
1) mpw3DataCollector
2) mpw3FileWriter
3) mpw3FileReader
4) mpw3FrameEvent2StdEventConverter
5) mpw3PreprocessedEvent2StdEventConverter

##The modules

##1) mpw3DataCollector

Is no typical DataCollector as we do not receive Events from a EUDAQ-Producer but set up our own socket, within which we receive UDP-packages 
with a data rate of up to ~800MBit/s. These packges are sent directly by the FPGA (jumbo frames with 8000 byte payload).
Generates "MPW3FrameEvent"-events from whole Chip frames (SOF, n * 32bit data words, EOF).

Usage: start "euCliCollector" with the parameters "-n MPW3DataCollector -t mpw3_dc -r <RunControlIP>"

##2) mpw3FileWriter

Just a native EUDAQ-FileWriter but storing to ".mpw3raw" files instead of ".raw" files.
To be used with the DataCollector

Usage: specify "EUDAQ_FW = mpw3_fw" in the config section of the DataCollector

##3) mpw3FileReader

As the MPW3FrameEvent's do not yet contain "real" events this module is generating a global-Timestamps for each hit and
merging them into "MPW3PreprocessedEvent"s (this time "real" events).
Is getting invoked by euCliConverter when input is a ".mpw3raw"-file.

Usage: Call "euCliConverter" with an ".mpw3raw" input file, output file should be of type ".raw". Eg "euCliConverter -i xxx.mpw3raw -o yyy.raw"


##4) mpw3FrameEvent2StdEventConverter

Converts a "Mpw3FrameEvent" into an EudaqStandard-Event. As as mentioned above the "Mpw3FrameEvent" is no "real" event
 the conversion should only be done when using the mpw3DataCollector with a monitor to get online information during a run.
rd50_mpw3_gui is utilizing this converter.

##5) mpw3PreprocessedEvent2StdEventConverter

Converts a "MPW3PreprocessedEvent" into an EudaqStandard-Event. This converter is be used by Corryvreckan when loading a preprocessed ".raw"-file with [EventLoaderEUDAQ2]
