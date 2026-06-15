#!/usr/bin/env bash
# init_hsr.sh — Inicializa HSR vía SSH (sin expect)

set -e

# ========= CONFIG =========
HOST="hsrb.local"
USER="administrator"
PASSWORD="password"
ROS_DOMAIN_ID="1"

STOP_CMD="sh stop_ros2_docker.sh"
START_CMD="sh start_ros2_docker.sh"
# ==========================

sshpass -p "$PASSWORD" ssh -x -o StrictHostKeyChecking=accept-new "${USER}@${HOST}" <<REMOTE
set -e

${STOP_CMD}
sleep 2
echo "${ROS_DOMAIN_ID}" | ${START_CMD}

exit
REMOTE

echo "✅ HSR inicializado (ROS_DOMAIN_ID=${ROS_DOMAIN_ID})"
