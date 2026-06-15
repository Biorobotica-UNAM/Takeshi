#!/usr/bin/env bash
# init_hsr.sh — Inicializa HSR vía SSH (sin expect)

set -e

# ========= CONFIG =========
HOST="hsrb.local"
USER="administrator"
PASSWORD="password"

sshpass -p "$PASSWORD" ssh -x -o StrictHostKeyChecking=accept-new "${USER}@${HOST}" <<REMOTE
set -e

for i in /sys/bus/pci/drivers/[ex]hci_hcd/*:*; do
  [ -e "$i" ] || continue
  echo "${i##*/}" | sudo tee /sys/bus/pci/drivers/[ex]hci_hcd/unbind
  sleep 1
  echo "${i##*/}" | sudo tee /sys/bus/pci/drivers/[ex]hci_hcd/bind
done

exit
REMOTE
