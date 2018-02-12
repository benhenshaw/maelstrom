cc maelstrom.c -framework SDL2 -o maelstrom -Wall -framework OpenGL
if [[ $? -eq 0 ]]; then
    ./maelstrom
fi
