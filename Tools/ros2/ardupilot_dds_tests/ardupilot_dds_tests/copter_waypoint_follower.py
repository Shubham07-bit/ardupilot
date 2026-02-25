#!/usr/bin/env python3
# Copyright...
# GNU License...

"""
Copter takeoff and GPS navigation demo.
"""

import rclpy
import time
import math
from rclpy.node import Node
from geographic_msgs.msg import GeoPoseStamped
from ardupilot_msgs.srv import ArmMotors, ModeSwitch, Takeoff
from ardupilot_msgs.msg import GlobalPosition

COPTER_MODE_GUIDED = 4
TAKEOFF_ALT = 10.0  # meters

# GPS Goal
GOAL_LAT = -35.362194
GOAL_LON = 149.164678
GOAL_ALT = 20.0  # meters

class CopterTakeoffAndNavigate(Node):
    def __init__(self):
        super().__init__("copter_waypoint_follower")
        
        self._home_alt = None

        # Parameters and services
        self._client_arm = self.create_client(ArmMotors, "/ap/arm_motors")
        self._client_mode_switch = self.create_client(ModeSwitch, "/ap/mode_switch")
        self._client_takeoff = self.create_client(Takeoff, "/ap/experimental/takeoff")
        self._gps_pub = self.create_publisher(GlobalPosition, "/ap/cmd_gps_pose", 10)

        # Wait for services
        for client, name in [(self._client_arm, "arm"), (self._client_mode_switch, "mode_switch"), (self._client_takeoff, "takeoff")]:
            while not client.wait_for_service(timeout_sec=1.0):
                self.get_logger().info(f'{name} service not available, waiting again...')

        self._cur_geopose = GeoPoseStamped()
        qos = rclpy.qos.QoSProfile(
                    reliability=rclpy.qos.ReliabilityPolicy.BEST_EFFORT,
                    durability=rclpy.qos.DurabilityPolicy.VOLATILE,
                    depth=10
                )
        self.create_subscription(GeoPoseStamped, "/ap/geopose/filtered", self.geopose_cb, qos)

    def geopose_cb(self, msg: GeoPoseStamped):
        self._cur_geopose = msg
        if self._home_alt is None:
            self._home_alt = msg.pose.position.altitude
            self.get_logger().info(f"🏠 Home altitude set to {self._home_alt:.2f}m")


    def arm(self):
        req = ArmMotors.Request()
        req.arm = True
        future = self._client_arm.call_async(req)
        rclpy.spin_until_future_complete(self, future)
        return future.result()

    def arm_with_timeout(self, timeout_sec: float):
        start = time.time()
        while time.time() - start < timeout_sec:
            if self.arm().result:
                self.get_logger().info("Armed successfully")
                return True
            time.sleep(1)
        return False

    def switch_mode(self, mode: int):
        req = ModeSwitch.Request()
        req.mode = mode
        future = self._client_mode_switch.call_async(req)
        rclpy.spin_until_future_complete(self, future)
        return future.result()

    def switch_mode_with_timeout(self, mode: int, timeout_sec: float):
        start = time.time()
        while time.time() - start < timeout_sec:
            result = self.switch_mode(mode)
            if result.status or result.curr_mode == mode:
                self.get_logger().info(f"Mode switched to {mode}")
                return True
            time.sleep(1)
        return False

    def takeoff(self, alt: float):
        req = Takeoff.Request()
        req.alt = alt
        future = self._client_takeoff.call_async(req)
        rclpy.spin_until_future_complete(self, future)
        return future.result()

    def takeoff_with_timeout(self, alt: float, timeout_sec: float):
        start = time.time()
        while time.time() - start < timeout_sec:
            result = self.takeoff(alt)
            if result.status:
                self.get_logger().info("Takeoff command accepted")
                return True
            time.sleep(1)
        return False

    def reached_altitude(self, target_rel_alt: float, threshold: float = 1.0):
        if self._home_alt is None:
            return False  # Wait until home altitude is set

        current_abs_alt = self._cur_geopose.pose.position.altitude
        rel_alt = current_abs_alt - self._home_alt
        self.get_logger().info(f"Current relative altitude: {rel_alt:.2f}m (target: {target_rel_alt}m)")
        return abs(rel_alt - target_rel_alt) < threshold

    def send_goal(self, lat: float, lon: float, alt: float):
        msg = GlobalPosition()
        msg.latitude = lat
        msg.longitude = lon
        msg.altitude = alt / 1000.0  # convert to km for ArduPilot
        msg.FRAME_GLOBAL_INT = 5  # FRAME_GLOBAL_INT
        self._gps_pub.publish(msg)
        self.get_logger().info(f"Sent goal: {lat:.6f}, {lon:.6f}, {alt}m")

    def goal_reached(self, lat, lon, alt, threshold=2.0):
        pos = self._cur_geopose.pose.position
        d_lat = (lat - pos.latitude) * 1e5
        d_lon = (lon - pos.longitude) * 1e5
        d_alt = alt - pos.altitude
        dist = math.sqrt(d_lat**2 + d_lon**2 + d_alt**2)
        return dist < threshold

def main(args=None):
    rclpy.init(args=args)
    node = CopterTakeoffAndNavigate()

    try:
        if not node.switch_mode_with_timeout(COPTER_MODE_GUIDED, 20):
            raise RuntimeError("Unable to switch to GUIDED")
        if not node.arm_with_timeout(30):
            raise RuntimeError("Unable to arm")
        if not node.takeoff_with_timeout(TAKEOFF_ALT, 20):
            raise RuntimeError("Takeoff failed")

        node.get_logger().info("Ascending to target altitude...")
        while not node.reached_altitude(TAKEOFF_ALT):
            rclpy.spin_once(node)
            time.sleep(1)

        node.get_logger().info("Reached takeoff altitude. Sending GPS goal.")
        node.send_goal(GOAL_LAT, GOAL_LON, GOAL_ALT)

        while not node.goal_reached(GOAL_LAT, GOAL_LON, GOAL_ALT):
            rclpy.spin_once(node)
            time.sleep(1)

        node.get_logger().info("✅ Reached GPS Goal!")

    except KeyboardInterrupt:
        node.get_logger().info("Interrupted")
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == "__main__":
    main()
