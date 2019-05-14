#! /bin/bash

# start gatt server
./bluez-5.43/test/example-gatt-server &

# broadcast ble
./broadcast_bt.sh
