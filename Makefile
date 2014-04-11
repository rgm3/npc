all: npc

npc: npc.c
	gcc -O3 -lcrypto -lssl -lcurl npc.c -o npc 
