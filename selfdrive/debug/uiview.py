#!/usr/bin/env python3
import argparse
import math
import time
from cereal import car, log, messaging
from common.params import Params
from selfdrive.hardware import HARDWARE
from selfdrive.manager.process_config import managed_processes

KPH_TO_MS = 1.0 / 3.6


def main():
  parser = argparse.ArgumentParser(description="Launch onroad UI with camera + Optech HUD preview")
  parser.add_argument("--cycle", action="store_true",
                      help="Cycle through cruising, turn/camera, red-light stop, and green departure states")
  parser.add_argument("--no-lead", action="store_true", help="Disable simulated lead vehicle / chevron")
  parser.add_argument("--dist", type=float, default=30.0, help="Simulated lead distance in meters (default: 30)")
  parser.add_argument("--speed", type=float, default=66.0, help="Simulated ego speed in km/h (default: 66)")
  args = parser.parse_args()

  params = Params()
  CP = car.CarParams(notCar=True, openpilotLongitudinalControl=True, sccBus=2)
  params.put("CarParams", CP.to_bytes())
  if not params.get("ShowHudMode"):
    params.put("ShowHudMode", "7")
  if not params.get("IsMetric"):
    params.putBool("IsMetric", True)

  procs = ['camerad', 'ui', 'modeld', 'calibrationd']
  HARDWARE.set_power_save(False)
  for p in procs:
    managed_processes[p].start()

  services = [
    'controlsState', 'deviceState', 'pandaStates', 'carParams',
    'carState', 'carControl', 'radarState', 'longitudinalPlan',
    'roadLimitSpeed', 'gpsLocationExternal',
  ]
  pm = messaging.PubMaster(services)

  t0 = time.monotonic()
  try:
    while True:
      t = time.monotonic() - t0
      phase = int(t / 6.0) % 4 if args.cycle else 0

      # Scenario values
      v_kph = args.speed
      a_ego = 0.35
      lead_d = args.dist
      lead_kph = 82.0
      traffic_state = 2  # cruising green (verified hidden above 3 m/s)
      x_state = log.LongitudinalPlan.XState.lead
      turn_info, turn_dist = 0, 0
      cam_spd, cam_dist = 0, 0
      lb, rb = False, False

      if args.cycle:
        if phase == 1:  # Approaching turn + speed camera + right blinker
          v_kph, a_ego = 64.0, -0.4
          lead_d, lead_kph = 24.0, 60.0
          turn_info, turn_dist = 2, 140
          cam_spd, cam_dist = 60, 320
          rb = True
        elif phase == 2:  # Stopped at red light behind close lead + left blinker
          v_kph, a_ego = 0.0, 0.0
          lead_d, lead_kph = 5.5, 0.0
          traffic_state = 1
          x_state = log.LongitudinalPlan.XState.e2eStop
          turn_info, turn_dist = 1, 25
          lb = True
        elif phase == 3:  # Green light departure (ramps 0 -> 18 km/h; green light auto-hides past ~11 km/h)
          sub_t = (t % 6.0)
          v_kph = min(18.0, sub_t * 4.0)
          a_ego = 0.8
          lead_d = 6.0 + sub_t * 3.5
          lead_kph = min(28.0, sub_t * 6.0)
          traffic_state = 2
          x_state = log.LongitudinalPlan.XState.e2eCruisePrepare if v_kph < 5.0 else log.LongitudinalPlan.XState.lead

      v_ms = v_kph * KPH_TO_MS
      lead_ms = lead_kph * KPH_TO_MS

      # 1. deviceState
      msg = messaging.new_message('deviceState')
      msg.deviceState.started = True
      msg.deviceState.cpuTempC = [48.0, 49.0, 47.0, 48.0]
      msg.deviceState.cpuUsagePercent = [62, 58, 65, 55]
      pm.send('deviceState', msg)

      # 2. pandaStates
      msg = messaging.new_message('pandaStates', 1)
      msg.pandaStates[0].ignitionLine = True
      msg.pandaStates[0].pandaType = log.PandaState.PandaType.uno
      pm.send('pandaStates', msg)

      # 3. carParams
      msg = messaging.new_message('carParams')
      msg.carParams.openpilotLongitudinalControl = True
      msg.carParams.sccBus = 2
      pm.send('carParams', msg)

      # 4. carState
      msg = messaging.new_message('carState')
      msg.carState.vEgo = v_ms
      msg.carState.vEgoCluster = v_ms
      msg.carState.aEgo = a_ego
      msg.carState.standstill = v_ms < 0.05
      msg.carState.gearShifter = car.CarState.GearShifter.drive
      msg.carState.cruiseGap = 4
      msg.carState.leftBlinker = lb
      msg.carState.rightBlinker = rb
      msg.carState.steeringAngleDeg = 4.0 * math.sin(t * 0.8)
      msg.carState.tpms.fl = 36.0
      msg.carState.tpms.fr = 36.0
      msg.carState.tpms.rl = 35.0
      msg.carState.tpms.rr = 35.0
      pm.send('carState', msg)

      # 5. carControl
      msg = messaging.new_message('carControl')
      msg.carControl.enabled = True
      pm.send('carControl', msg)

      # 6. controlsState
      msg = messaging.new_message('controlsState')
      msg.controlsState.state = log.ControlsState.OpenpilotState.enabled
      msg.controlsState.enabled = True
      msg.controlsState.active = True
      msg.controlsState.longActiveUser = 1
      msg.controlsState.vCruise = 70.0
      msg.controlsState.vCruiseCluster = 70.0
      msg.controlsState.vCruiseOut = 60.0 if cam_spd > 0 else 70.0
      msg.controlsState.longCruiseGap = 4
      pm.send('controlsState', msg)

      # 7. longitudinalPlan
      msg = messaging.new_message('longitudinalPlan')
      msg.longitudinalPlan.hasLead = not args.no_lead
      msg.longitudinalPlan.trafficState = traffic_state
      msg.longitudinalPlan.xState = x_state
      msg.longitudinalPlan.cruiseGap = 4.0
      msg.longitudinalPlan.tFollow = 1.45
      msg.longitudinalPlan.accels = [a_ego] * 13
      msg.longitudinalPlan.speeds = [v_ms] * 13
      msg.longitudinalPlan.jerks = [0.0] * 13
      pm.send('longitudinalPlan', msg)

      # 8. radarState (primary lead + optional adjacent-lane radar object to verify non-overlap)
      msg = messaging.new_message('radarState')
      if not args.no_lead:
        l1 = msg.radarState.leadOne
        l1.status = True
        l1.radar = True
        l1.dRel = lead_d
        l1.yRel = 0.0
        l1.vRel = lead_ms - v_ms
        l1.vLead = lead_ms
        l1.vLeadK = lead_ms
        l1.modelProb = 0.95

        # Include leadOne in leadsCenter as real radard does, plus an adjacent left-lane car
        msg.radarState.init('leadsCenter', 1)
        msg.radarState.leadsCenter[0].dRel = lead_d
        msg.radarState.leadsCenter[0].yRel = 0.1
        msg.radarState.leadsCenter[0].vLeadK = lead_ms
        msg.radarState.leadsCenter[0].status = True

        msg.radarState.init('leadsLeft', 1)
        msg.radarState.leadsLeft[0].dRel = 22.0
        msg.radarState.leadsLeft[0].yRel = 3.4
        msg.radarState.leadsLeft[0].vLeadK = (v_kph + 8.0) * KPH_TO_MS
        msg.radarState.leadsLeft[0].status = True
      pm.send('radarState', msg)

      # 9. roadLimitSpeed
      msg = messaging.new_message('roadLimitSpeed')
      msg.roadLimitSpeed.active = 1
      msg.roadLimitSpeed.roadLimitSpeed = 60
      msg.roadLimitSpeed.camLimitSpeed = cam_spd
      msg.roadLimitSpeed.camLimitSpeedLeftDist = cam_dist
      msg.roadLimitSpeed.xTurnInfo = turn_info
      msg.roadLimitSpeed.xDistToTurn = turn_dist
      pm.send('roadLimitSpeed', msg)

      # 10. gpsLocationExternal
      msg = messaging.new_message('gpsLocationExternal')
      msg.gpsLocationExternal.flags = 1
      msg.gpsLocationExternal.speed = max(0.0, (v_kph - 3.0) * KPH_TO_MS)
      msg.gpsLocationExternal.accuracy = 2.5
      pm.send('gpsLocationExternal', msg)

      time.sleep(0.05)  # 20 Hz
  except KeyboardInterrupt:
    for p in procs:
      managed_processes[p].stop()


if __name__ == "__main__":
  main()
