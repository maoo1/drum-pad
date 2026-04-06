import socket
import pygame

pygame.mixer.pre_init(44100, -16, 2, 256)
pygame.init()

drum_center = pygame.mixer.Sound("center-hit.wav")
drum_rim = pygame.mixer.Sound("rim.wav")
drum_mute = pygame.mixer.Sound("mute-hit.wav");
cymbal_hit = pygame.mixer.Sound("cymbal.wav")
cymbal_edge = pygame.mixer.Sound("cymbal-mute.wav")
gong_hit = pygame.mixer.Sound("center-hit.wav")
gong_edge = pygame.mixer.Sound("center-hit.wav")

drum_center.set_volume(0.6)
drum_rim.set_volume(0.5)
drum_mute.set_volume(0.5)
cymbal_hit.set_volume(0.6)
cymbal_edge.set_volume(0.5)
gong_hit.set_volume(0.7)
gong_edge.set_volume(0.6)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", 5005))

print("Listening for ESP32 UDP messages on port 5005...")

while True:
    data, addr = sock.recvfrom(1024)
    msg = data.decode().strip()
    print(msg)

    if msg == "DRUM_CENTER":
        drum_center.play()
    elif msg == "DRUM_RIM":
        drum_rim.play()
    elif msg == "DRUM_CENTER_MUTE":
        drum_mute.play()
    elif msg == "CYMBAL_HIT":
        cymbal_hit.play()
    elif msg == "CYMBAL_EDGE":
        cymbal_edge.play()
    elif msg == "GONG_HIT":
        gong_hit.play()
    elif msg == "GONG_EDGE":
        gong_edge.play()
    elif msg.startswith("MODE_"):
        print("Mode changed:", msg)