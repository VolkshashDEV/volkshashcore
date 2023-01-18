#!/bin/bash
# use testnet settings,  if you need mainnet,  use ~/.ukkeycore/ukkeyd.pid file instead
ukkey_pid=$(<~/.ukkeycore/testnet3/ukkeyd.pid)
sudo gdb -batch -ex "source debug.gdb" ukkeyd ${ukkey_pid}
