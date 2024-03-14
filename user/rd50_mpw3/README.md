# Modules for the RD50_MPW3 chip, in particular the "fast-data-path"

The following modules can be used for both the RD50-MPW3 as well as the RD50-MPW4.

## Building

By default, the CMake flag is deactivated `USER_MPW3_BUILD`. When you activate it with the CMake parameter `-D USER_MPW3_BUILD=ON`, several modules will be built:

- Mpw3DataCollector
- Mpw3Raw2StdEventConverter

## The Modules

### Mpw3DataCollector

Mpw3DataCollector is not a typical DataCollector as we do not receive events from a EUDAQ-Producer but set up our own socket, within which we receive UDP packages with a data rate of up to ~800MBit/s. These packages are sent directly by the FPGA (jumbo frames with 8000 byte payload). Generates "RD50_Mpw3Event"-events.

The config parameter "SYNC_MODE" specifies how the UDP packages are split into EUDAQ events. The following values set the synchronization mode accordingly:

- 0: Split from SOF to EOF
- 1: Split on trigger words from FPGA base FIFO
- 2: Split on trigger words from FPGA piggy FIFO

Usage: start "euCliCollector" with the parameters `-n MPW3DataCollector -t mpw3_dc -r <RunControlIP>`. The local Linux machine needs a bit of configuration too. You should execute a shell script something like the following before using the DataCollector:

```bash
TX_QUEUE_SIZE=10000
SOCKET_READ_BUFFER=8388608
sudo ip link set <iface> txqueuelen ${TX_QUEUE_SIZE}
sudo sysctl -w net.core.rmem_max=${SOCKET_READ_BUFFER}
sudo ip link set <iface> mtu 8196
```

### Mpw3Raw2StdEventConverter

Mpw3Raw2StdEventConverter converts the raw "RD50_MPWxEvent" from the DC to an EUDAQ standard event. Data from the base and piggy will be placed into two different planes.

#### Usage within Corryvreckan

To access the Base plane, one should use the following name in the Corryvreckan geometry: "RD50_MPWx_base_0". For the Piggy, it is "RD50_MPWx_piggy_0".

#### Parameters

The following parameters can be forwarded to the EventConverter:

- "filter_zeros": Do not process and interpret words which are plain "0". Such words would be interpreted as pixel 00:00 got hit with TS-LE = TS-TE = 0, defaults to "true".
- "t0_skip_time": All events are skipped (converter returns false to indicate an invalid event) until an event with a timestamp < the specified time in [$\mu s$] is encountered. Can be used to avoid passing events which have been received before the T0 signal of the TLU was raised, which would contain uninitialized timestamps and thereby not work properly with the EUDAQ2EventLoader module of Corryvreckan. Defaults to "-1.0", which means not skipping any events.
- "lsb_time": Specify the time one timestamp LSB corresponds to. Must be given in [ps]. Should be set to _25000_ for MPW4 (25ns time binning) and _50000_ for MPW3 (50ns time binning). Defaults to _50000_ (MPW3).
