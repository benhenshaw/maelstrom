cc maelstrom.c -framework SDL2 -o maelstrom -Wall
if [[ $? -eq 0 ]]; then
    ./maelstrom
fi
