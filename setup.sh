

#sudo apt update
#sudo apt install -y ros-humble-rmw-cyclonedds-cpp
#export ROS_DOMAIN_ID=1 >> ~/.bashrc
#export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp >> ~/.bashrc
#export CYCLONEDDS_URI=/cyclonedds_profile.xml >> ~/.bashrc
#source /opt/ros/humble/setup.bash >> ~/.bashrc
#source ~/.bashrc


#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "🔄 Actualizando paquetes..."
sudo apt update

echo "📦 Instalando CycloneDDS..."
sudo apt install -y ros-humble-rmw-cyclonedds-cpp

echo "⚙️ Configurando variables de entorno..."
grep -qxF 'export ROS_DOMAIN_ID=1' ~/.bashrc || echo 'export ROS_DOMAIN_ID=1' >> ~/.bashrc
grep -qxF 'export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp' ~/.bashrc || echo 'export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp' >> ~/.bashrc
grep -qxF 'export CYCLONEDDS_URI=~/cyclonedds_profile.xml' ~/.bashrc || echo 'export CYCLONEDDS_URI=/cyclonedds_profile.xml' >> ~/.bashrc
grep -qxF 'source /opt/ros/humble/setup.bash' ~/.bashrc || echo 'source /opt/ros/humble/setup.bash' >> ~/.bashrc

run_nested_tool_scripts() {
    local base_dir="${1:-.}"
    local current_script
    current_script="$(realpath "${BASH_SOURCE[0]}")"

    echo "🔎 Buscando scripts en carpetas *_tools dentro de: $base_dir"

    mapfile -t tool_scripts < <(
        find "$base_dir" -type f \( -name "install.sh" -o -name "setup.sh" \) \
            | grep '/[^/]*_tools/' \
            | sort
    )

    if [ ${#tool_scripts[@]} -eq 0 ]; then
        echo "ℹ️ No se encontraron scripts en carpetas *_tools"
        return 0
    fi

    for script in "${tool_scripts[@]}"; do
        [ "$(realpath "$script")" = "$current_script" ] && continue

        echo
        echo "▶ Ejecutando: $script"
        (
            cd "$(dirname "$script")"
            bash "./$(basename "$script")"
        )
    done
}
run_nested_tool_scripts "$SCRIPT_DIR/src"

echo "✅ Instalación completa."
echo "⚠️ Abre una nueva terminal o ejecuta: source ~/.bashrc"