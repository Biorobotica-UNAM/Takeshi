#!/bin/bash
echo "Configurando receptor en PC..."

# 1. Crear el Sink Nulo (El receptor de red)
pactl load-module module-null-sink \
    sink_name=robot_bridge \
    sink_properties=device.description="Robot_Receiver_Sink"

# 2. Crear la Fuente Virtual (El Micrófono que verán tus apps)
# Usamos el monitor del sink anterior como fuente
pactl load-module module-remap-source \
    master=robot_bridge.monitor \
    source_name=robot_mic \
    source_properties=device.description="Microfono_Virtual_Robot"

# 3. Habilitar TCP (Asegúrate de que este puerto esté abierto en tu firewall si tienes uno)
pactl load-module module-native-protocol-tcp auth-anonymous=1 auth-ip-acl=169.254.11.192