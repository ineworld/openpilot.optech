#!/usr/bin/env python3

import os
import time
import cereal.messaging as messaging
from common.params import Params

DEFAULT_SHUTDOWN_S = 60 * 30


def get_shutdown_at(params: Params) -> int:
  val = params.get("OptechAutoShutdown", encoding="utf8")
  if val is None or val == "":
    return DEFAULT_SHUTDOWN_S
  try:
    return int(val)
  except (TypeError, ValueError):
    return DEFAULT_SHUTDOWN_S


def shutdown():
  os.system('LD_LIBRARY_PATH="" svc power shutdown')


def main():
  # conflate: only the latest deviceState counts (this loop wakes every 5 s, deviceState comes at 2 Hz)
  device_state_sock = messaging.sub_sock('deviceState', conflate=True)
  params = Params()
  shutdown_since = None

  while 1:
    msg = messaging.recv_sock(device_state_sock, wait=True)
    now = time.monotonic()
    shutdown_at = get_shutdown_at(params)

    if msg is not None and shutdown_at > 0:
      ds = msg.deviceState
      if not ds.usbOnline and not ds.started:
        shutdown_since = shutdown_since or now
      else:
        shutdown_since = None
    else:
      shutdown_since = None

    elapsed = 0 if shutdown_since is None else int(now - shutdown_since)
    print('current', elapsed, 'shutdown_at', shutdown_at)

    if shutdown_since is not None and (now - shutdown_since) >= shutdown_at > 0:
      shutdown()

    time.sleep(5)


if __name__ == "__main__":
  main()
