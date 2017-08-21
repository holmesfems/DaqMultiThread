DaqMultiThread
==============
read DAQ device via multithread method and control by TCP connection

# Description

This tool is to read voltage data from DAQ device which monitor Beam-Switch
signal and One-Pulse-Per-Second(OPPS) signal via two channels. This tool records
these signals in binary form, that is, only save the state that is ON or OFF.
This format will save a lot of HDD space than just records raw data while the
sampling rate is about 80000 samples per second.

When run this application as server mode, it will start a thread that receive
the command via TCP connection. It can be controlled by other TCP client or using
the client mode of this application. For example, it will records the binary data
while receiving "read" command, which can be decoded by "decode mode" of this application.

# Requirement

# Download and install


# Usage
To start server mode and accept commands by TCP, run:


# Realease note:
2017/08/20 0.0 First version