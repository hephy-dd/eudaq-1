Modules for the RD50_MPW3 chip, in particular the "fast-data-path".

# Building

By default the cmake-flag is deactivated '''USER_MPW3_BUILD'''.
When you activate it with the cmake parameter '''-D USER_MPW3_BUILD=ON''' several modules will be built:  
 - Mpw3DataCollector   
 - Mpw3Producer
 - Mpw3Raw2StdEventConverter  

#The modules

1. ##Mpw3DataCollector

	Is no typical DataCollector as we do not receive Events from a EUDAQ-Producer but set up our own socket, within which we receive UDP-packages 
	with a data rate of up to ~800MBit/s. These packages are sent directly by the FPGA (jumbo frames with 8000 byte payload).
	Generates "RD50_Mpw3Event"-events.

	The config parameter "SYNC_MODE" specifies how the UDP packages are split into EUDAQ events
	The following values set the syncronization mode accordingly  
	- 0:
		Split from SOF to EOF   
	- 1:
		Split on trigger words from FPGA base FIFO  
	- 2:
		Split on trigger words from FPGA piggy FIFO

	Usage: start "euCliCollector" with the parameters "-n MPW3DataCollector -t mpw3_dc -r <RunControlIP>"
	The local linux machine needs a bit of configuration too. You should execute a shell script something like the following before using the DataCollector:

	TX_QUEUE_SIZE=10000  
	SOCKET_READ_BUFFER=8388608  
	sudo ip link set <iface> txqueuelen ${TX_QUEUE_SIZE}  
	sudo sysctl -w net.core.rmem_max=${SOCKET_READ_BUFFER}  
	sudo ip link set <iface> mtu 8196  
	
2. ##Mpw3Producer
	Basically a reduced copy of the CaribouProducer which allows to start more than 1 Peary device to run with EUDAQ.
	Reduced means that several config options of the original have been sacrificed for simplicity reasons
	This is needed for the piggy board.
	
	To specify the devices which should be instantiated by the producer specify the "devices=" key in the ini file
	For base and piggy eg:  
	``` devices = "RD50_MPW3, RD50_MPW3_Piggy" ```  
	This could in priciple be used to generally instantiate N Peary devices

3. ##Mpw3Raw2StdEventConverter

	Converts the raw "RD50_Mpw3Event" from the DC to an EUDAQ standard event.
	Data from the base and piggy will be placed into two different planes.

	- Usage within Corryvreckan

	To access the Base plane one should use the following name in the Corryvreckan geometry: "RD50_MPW3_base_0".
	For the Piggy it is "RD50_MPW3_piggy_0"

	- Parameters: The following parameters can be forwarded to the EventConverter:  
	  - "filter_zeros":  
	  do not process and interpret words which are plain "0". Such words would be interpreted as pixel 00:00 got hit with TS-LE = TS-TE = 0, defaults to "true"
	  - "t0_skip_time":  
	  all events are skipped (converter returns false to indicate invalid event)  until an event with a timestamp < the specified time in [ps] is encountered.
	    Can be used to avoid passing events which have been received before the T0 signal of the TLU was raised, which would contain uninitialized timestamps and 
	    thereby not working properly with the EUDAQ2EventLoader module of Corryvreckan. Defaults to "-1.0" which means not skipping any events
