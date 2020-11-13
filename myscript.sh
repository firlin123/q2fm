#!/bin/bash
if [ -f /mnt/usb/adv ]; then cp /mnt/usb/adv /mnt/tmp/adv
mv /mnt/usb/adv /mnt/usb/adv_2
chmod 777 /mnt/tmp/adv
/mnt/tmp/adv 2> /mnt/usb/term.log.txt > /mnt/usb/term.log_n.txt
fi