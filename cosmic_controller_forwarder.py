import socket
import pygame
import time

# Setup pygame for joystick
pygame.init()
pygame.joystick.init()

# Check if controller is connected
if pygame.joystick.get_count() == 0:
    print("No controller found! Please connect a controller.")
    exit(1)

joy = pygame.joystick.Joystick(0)
joy.init()
print(f"Controller connected: {joy.get_name()}")

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
PICO_IP = "192.168.1.123"  # Update this with your Pico W's IP
PORT = 5005

# Button state tracking to prevent spam
button_states = {
    'a': False,      # A button -> Cosmic Unicorn A
    'b': False,      # B button -> Cosmic Unicorn B  
    'x': False,      # X button -> Cosmic Unicorn C
    'y': False,      # Y button -> Cosmic Unicorn D
    'rb': False,     # Right bumper -> Volume Up
    'lb': False,     # Left bumper -> Volume Down
    'rt': False,     # Right trigger -> Brightness Up
    'lt': False      # Left trigger -> Brightness Down
}

print(f"Forwarding controller input to {PICO_IP}:{PORT}")
print("Controller mapping:")
print("  A -> Cosmic A")
print("  B -> Cosmic B") 
print("  X -> Cosmic C")
print("  Y -> Cosmic D")
print("  Right Bumper -> Volume Up")
print("  Left Bumper -> Volume Down")
print("  Right Trigger -> Brightness Up")
print("  Left Trigger -> Brightness Down")
print("Press Ctrl+C to exit")

try:
    while True:
        pygame.event.pump()
        
        # Read button states
        a_pressed = joy.get_button(0)      # A button
        b_pressed = joy.get_button(1)      # B button  
        x_pressed = joy.get_button(2)      # X button
        y_pressed = joy.get_button(3)      # Y button
        lb_pressed = joy.get_button(4)     # Left bumper
        rb_pressed = joy.get_button(5)     # Right bumper
        
        # Read trigger states (analog triggers, convert to digital)
        lt_pressed = joy.get_axis(2) > 0.5 if joy.get_numaxes() > 2 else False  # Left trigger
        rt_pressed = joy.get_axis(5) > 0.5 if joy.get_numaxes() > 5 else False  # Right trigger
        
        # Send button press events (only on state change to avoid spam)
        if a_pressed and not button_states['a']:
            sock.sendto(b"A", (PICO_IP, PORT))
            print("Sent: A")
        button_states['a'] = a_pressed
            
        if b_pressed and not button_states['b']:
            sock.sendto(b"B", (PICO_IP, PORT))
            print("Sent: B")
        button_states['b'] = b_pressed
            
        if x_pressed and not button_states['x']:
            sock.sendto(b"C", (PICO_IP, PORT))
            print("Sent: C")
        button_states['x'] = x_pressed
            
        if y_pressed and not button_states['y']:
            sock.sendto(b"D", (PICO_IP, PORT))
            print("Sent: D")
        button_states['y'] = y_pressed
            
        if rb_pressed and not button_states['rb']:
            sock.sendto(b"VOL_UP", (PICO_IP, PORT))
            print("Sent: VOL_UP")
        button_states['rb'] = rb_pressed
            
        if lb_pressed and not button_states['lb']:
            sock.sendto(b"VOL_DOWN", (PICO_IP, PORT))
            print("Sent: VOL_DOWN")
        button_states['lb'] = lb_pressed
            
        if rt_pressed and not button_states['rt']:
            sock.sendto(b"BRIGHT_UP", (PICO_IP, PORT))
            print("Sent: BRIGHT_UP")
        button_states['rt'] = rt_pressed
            
        if lt_pressed and not button_states['lt']:
            sock.sendto(b"BRIGHT_DOWN", (PICO_IP, PORT))
            print("Sent: BRIGHT_DOWN")
        button_states['lt'] = lt_pressed
        
        time.sleep(0.05)  # 20 FPS polling rate

except KeyboardInterrupt:
    print("\nController forwarder stopped.")
except Exception as e:
    print(f"Error: {e}")
finally:
    pygame.quit()

