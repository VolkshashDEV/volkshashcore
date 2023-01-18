#!/bin/bash
# use testnet settings,  if you need mainnet,  use ~/.volkshashcore/volkshashd.pid file instead
volkshash_pid=$(<~/.volkshashcore/testnet3/volkshashd.pid)
sudo gdb -batch -ex "source debug.gdb" volkshashd ${volkshash_pid}
