import rclpy
from rclpy.node import Node
from .robot import Robot
from .restaurant_tasks import RestaurantTasks

class RestaurantFSM:
    def __init__(self, node):
        self.node = node
        self.robot = Robot(node)
        self.tasks = RestaurantTasks(node, self.robot)
        
        # Almacén de datos de la misión
        self.orders = {}  # Ej: {'customer_1': 'coke'}
        self.locations = {
            'bar': 'kitchen_bar'
        }

    def run(self):
        """
        Ciclo principal de la tarea Restaurant.
        Tiempo máximo permitido: 15 minutos[cite: 91].
        """
        self.node.get_logger().info("--- INICIANDO RESTAURANT CHALLENGE ---")

        # 1. Fase Inicial: Guardar ubicación de la barra
        self.tasks.save_location(self.locations['bar'])
        self.robot.say(self.tasks.dialogs["start"])

        # 2. Ciclo de Clientes (Se requieren al menos 2 )
        for i in range(1, 3):
            customer_label = f"customer_{i}"
            success = self.execute_customer_cycle(customer_label)
            if not success:
                self.node.get_logger().error(f"Fallo en ciclo de {customer_label}, continuando...")

        # 3. Finalización
        self.robot.say(self.tasks.dialogs["finish"])
        self.node.get_logger().info("--- TAREA FINALIZADA ---")

    def execute_customer_cycle(self, label):
        """
        Mapeo de fases de CLIPS adaptado a ROS2 y el reglamento.
        """
        # --- DETECCIÓN (100 pts)  ---
        # Llama a tu Find.cpp (categoría waving)
        if not self.tasks.find_waving_customer():
            return False

        # --- NAVEGACIÓN A MESA (100 pts)  ---
        # Se mueve a la posición guardada por el nodo de visión
        if not self.robot.go_to("calling_customer"):
            return False

        # --- TOMA DE PEDIDO Y CONTACTO VISUAL (200 pts)  ---
        # Activamos focus inmediatamente después de llegar para evitar penalización de -80 pts 
        self.tasks.set_focus(True)
        
        order = self.tasks.get_order()
        if not order:
            self.tasks.set_focus(False)
            return False
        
        self.orders[label] = order
        self.tasks.save_location(label) # Guardamos la mesa para volver después
        self.tasks.set_focus(False)

        # --- COMUNICACIÓN AL BARMAN (100 pts)  ---
        self.robot.go_to(self.locations['bar'])
        self.robot.say(self.tasks.dialogs["to_barman"].format(order))

        # Aquí se usa el ObjectFinderNode de C++
        found = self.robot.tasks.find_object(order)
        
        if found and found.success:
            self.robot.hardware.close_gripper()
            # Opcional: Colocar en bandeja para bono de +200 pts 
            # self.tasks.place_on_tray() 
        else:
            # Si falla, el Barman puede entregarlo en la mano (-100 pts asistencia) 
            self.robot.say("Barman, please hand me the object.")

        # --- ENTREGA (200 pts)  ---
        self.robot.go_to(label)
        self.tasks.deliver_to_table(label)
        
        return True

def main(args=None):
    rclpy.init(args=args)
    # Importante: El nodo debe ser compatible con MultiThreadedExecutor para los servicios
    node = Node('restaurant_executor')
    fsm = RestaurantFSM(node)
    
    try:
        fsm.run()
    except Exception as e:
        node.get_logger().error(f"Excepción en FSM: {e}")
    finally:
        node.destroy_node()
        rclpy.shutdown()