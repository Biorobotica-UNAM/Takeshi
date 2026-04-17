import time
from .service_client_base import ServiceClientBase
from std_srvs.srv import SetBool
from known_locations_interfaces.srv import LocationsServer

class RestaurantTasks(ServiceClientBase):
    def __init__(self, node, robot):
        super().__init__(node)
        self.robot = robot
        # Mapeo de diálogos según el reglamento RoboCup@Home 2026
        self.dialogs = {
            "start": "I am ready for the restaurant challenge. I will wait for a customer.",
            "intro": "Hello! I am your robot waitress. What would you like to order?",
            "confirm": "So, you want {}, is that correct?",
            "to_barman": "Hello Barman, I have an order: {}.",
            "arrived_table": "I have arrived with your order. Enjoy your meal!",
            "finish": "Task complete. Returning to base."
        }
        # Clientes específicos para estas herramientas
        self.cli_save_loc = self.node.create_client(
            LocationsServer, 
            '/locations_manager/add'
        )
        self.cli_focus = self.node.create_client(
            SetBool, 
            '/vision/focus'
        )

    # =========================================================
    # SaveLoc (Traducción de Saveloc.cpp)
    # =========================================================
    def save_location(self, location_name: str) -> bool:
        """
        Guarda la pose actual del robot con un nombre específico en el Locations Manager.
        Vital para las Fases 3 y 6 de tu plan (guardar posición del cliente).
        """
        req = LocationsServer.Request()
        req.location_name = location_name
        
        self.node.get_logger().info(f"Guardando ubicación actual como: {location_name}")
        
        res = self.call_service(
            self.cli_save_loc, 
            req, 
            '/locations_manager/add',
            wait_timeout=3.0,
            call_timeout=5.0
        )
        
        if res:
            return res.success
        return False

    
    # =========================================================
    # Focus (Traducción de Focus.cpp)
    # =========================================================
    def set_focus(self, enable: bool) -> bool:
        """
        Activa/Desactiva el foco de visión.
        Útil para procesos de alta carga como reconocimiento de rostros o manipulación.
        """
        req = SetBool.Request()
        req.data = enable
        
        self.node.get_logger().info(f"Cambiando foco de visión a: {enable}")
        
        res = self.call_service(
            self.cli_focus, 
            req, 
            '/vision/focus',
            wait_timeout=3.0,
            call_timeout=5.0
        )
        
        if res:
            return res.success
        return False
        

    # =========================================================
    # Acciones de Visión (Basadas en tus nodos Find/Focus)
    # =========================================================
    
    def focus_vision(self, enable: bool) -> bool:
        """
        Llama al servicio /vision/focus (Equivalente a tu Focus.cpp).
        Activa el procesamiento de alto rendimiento en la GPU.
        """
        from std_srvs.srv import SetBool
        req = SetBool.Request()
        req.data = enable
        
        # Usamos el cliente genérico de vision que ya tienes en tu clase Vision
        res = self.call_service(self.robot.vision.find_object_cli, req, "/vision/focus")
        return res.success if res else False

    def find_waving_customer(self) -> bool:
        """
        Activa el PersonFinderNode para buscar gestos de llamado.
        Equivalente a la categoría 'waving' en tu Find.cpp.
        """
        self.robot.say(self.dialogs["start"])
        # Llamamos a find_person_cli con Trigger para activar la FSM de waving
        res = self.robot.find_waiving_client_call() 
        return res.success if res else False

    def find_and_track_object(self, object_name: str):
        """
        Utiliza el ObjectFinderNode (tu FSM de C++) para localizar 
        un objeto y que la cabeza lo siga (LookAtTarget).
        """
        return self.robot.tasks.find_object(object_name)

    # =========================================================
    # Acciones de HRI (Basadas en tu plan CLIPS)
    # =========================================================

    def get_order(self) -> str:
        """
        Ciclo completo: Preguntar -> Escuchar -> Confirmar.
        Cumple con 'Understand and confirm the order' (200 pts).
        """
        self.robot.say(self.dialogs["intro"])
        order = self.robot.listen(grammar="restaurant_items")
        
        if not order:
            self.robot.say("Sorry, I didn't hear you. Can you repeat?")
            order = self.robot.listen(grammar="restaurant_items")

        # Confirmación obligatoria según reglamento
        self.robot.say(self.dialogs["confirm"].format(order))
        confirmation = self.robot.listen(grammar="yes_no")
        
        if "yes" in confirmation.lower():
            return order
        return self.get_order() # Reintento si no confirmó

    # =========================================================
    # Acciones de Hardware & Manipulación
    # =========================================================

    def deliver_to_table(self, location_name: str):
        """Navega y entrega."""
        self.robot.go_to(location_name)
        self.robot.say(self.dialogs["arrived_table"])
        self.robot.hardware.open_gripper()
        time.sleep(1.0)
        self.robot.hardware.relax_gripper()