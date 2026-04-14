#!/bin/bash
PC_IP="169.254.11.200"
ROBOT_IP="169.254.11.192"
ROBOT_USER="administrator"
ROBOT_PASS="password"

echo "--- Iniciando puente de audio con GStreamer + Autenticación ---"

# 1. Detectar el micro en el robot de forma automática
ROBOT_SOURCE=$(sshpass -p "$ROBOT_PASS" ssh -o StrictHostKeyChecking=no $ROBOT_USER@$ROBOT_IP "pactl list sources short | grep 'alsa_input' | head -n 1 | cut -f2")
echo "Micrófono detectado: $ROBOT_SOURCE"

# 2. Lanzar receptor en PC (en segundo plano)
# Añadimos 'queue' para manejar el buffering y evitar los warnings de latencia
gst-launch-1.0 udpsrc port=5000 caps="application/x-rtp,media=(string)audio,clock-rate=(int)48000,encoding-name=(string)OPUS,payload=(int)96" ! \
    rtpopusdepay ! opusdec ! audioconvert ! audioresample ! \
    queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 min-threshold-time=20000000 ! \
    pulsesink device=robot_bridge &

RECEPTOR_PID=$!
sleep 1

# 3. Lanzar emisor en Robot
echo "Transmitiendo desde el robot..."
sshpass -p "$ROBOT_PASS" ssh -o StrictHostKeyChecking=no $ROBOT_USER@$ROBOT_IP \
    "gst-launch-1.0 pulsesrc device=$ROBOT_SOURCE ! \
    audioconvert ! audioresample ! opusenc bitrate=64000 ! \
    rtpopuspay ! udpsink host=$PC_IP port=5000"

# Al cerrar el script (Ctrl+C), matamos el receptor local
kill $RECEPTOR_PID