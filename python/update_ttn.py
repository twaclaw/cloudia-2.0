# Copyright (C) 2020-2022 Michael Kuyper. All rights reserved.
#
# This file is subject to the terms and conditions defined in file 'LICENSE',
# which is part of this source code package.

from zfwtool import Update
import frag
import asyncio

from base64 import b64encode
from hbmqtt.client import MQTTClient
from hbmqtt.mqtt.constants import QOS_0
import json
import os
import random
import struct
import sys

#  ------ Begin ENV variables ------
TTN_APPID = os.getenv("TTN_APPID")
if TTN_APPID is None:
    print("Please export TTN_APPID ENV var (e.g.: \"app-name@ttn\"")
    sys.exit(-1)

TTN_APPTOKEN = os.getenv("TTN_APPTOKEN")
if TTN_APPTOKEN is None:
    print("Please export TTN_APPTOKEN ENV var (e.g.: \"NNSXN.13234...\"")
    sys.exit(-1)

TTN_MQTTHOST = os.getenv("TTN_MQTTHOST")
if TTN_MQTTHOST is None:
    print("Please export TTN_MQTTHOST var (e.g.: \"nam1.cloud.thethings.network:1883\"")
    sys.exit(-1)
#  ------- End ENV variables --------


TTN_URI = f"mqtt://{TTN_APPID}:{TTN_APPTOKEN}@{TTN_MQTTHOST}/"
TTN_TUPLINK = f"v3/{TTN_APPID}/devices/+/up"

UPDATE_PORT = 16
# choose fragment size (multiple of 4, fragment data plus 8-byte header must fit LoRaWAN payload size!)
FRAG_SIZE = 128

#  Check usage
if len(sys.argv) != 2:
    print('usage: %s <update-file>' % sys.argv[0])
    exit()


async def main():
    # load update file
    updata = open(sys.argv[1], 'rb').read()
    up = Update.fromfile(updata)

    # short CRC of updated firmware
    dst_crc = up.fwcrc & 0xFFFF

    # short CRC of referenced firmware in case of delta update or 0
    src_crc = struct.unpack_from('<I', up.data)[0] & 0xFFFF if up.uptype == Update.TYPE_LZ4DELTA else 0

    # initialize fragment generator
    fc = frag.FragCarousel.fromfile(updata, FRAG_SIZE)

    #  #### setup MQTT
    ttn_client = MQTTClient('test')
    await ttn_client.connect(TTN_URI)
    await ttn_client.subscribe([(TTN_TUPLINK, QOS_0)])

    #  generate FUOTA downlinks
    while True:
        #  wait for uplink
        m = await ttn_client.deliver_message()
        data = json.loads(m.data.decode('utf-8'))
        print(f"MQTT IN > {m.topic}")
        devid = data['end_device_ids']['device_id']

        # randomly select non-zero fragment index
        idx = random.randint(1, 65535)
        # generate FUOTA payload (session header plus fragment data)
        payload = struct.pack('<HHHH', src_crc, dst_crc, fc.cct, idx) + fc.chunk(idx)

        #  schedule downlink
        dnmsg = {
            "downlinks": [
                {
                    "f_port": UPDATE_PORT,
                    "frm_payload": b64encode(payload).decode('ascii'),
                    "priority": "HIGH",
                    "confirmed": False
                }
            ]
        }

        topic = f"v3/{TTN_APPID}/devices/{devid}/down/replace"
        mqttmsg = json.dumps(dnmsg).encode('ascii')
        asyncio.create_task(ttn_client.publish(topic, mqttmsg, QOS_0))
        print(f"MQTT OUT < {topic} {mqttmsg}")


if __name__ == '__main__':
    asyncio.run(main())