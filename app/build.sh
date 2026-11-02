#!/bin/sh
# Gera app/index_html.h a partir de web/index.html e compila o app grafico.
#   ./build.sh            -> compila para o sistema atual (gcc)
#   ./build.sh release    -> gera Windows x64 + Linux x86_64 estatico (precisa do zig: pip install ziglang)
set -e
cd "$(dirname "$0")"
python3 - <<'PY'
d = open('web/index.html', 'rb').read()
open('index_html.h', 'w').write('static const unsigned char index_html[] = {\n' + ','.join(map(str, d)) +
                                '\n};\nstatic const unsigned long index_html_len = %d;\n' % len(d))
PY
SRC="../src/registro.c ../src/avl.c ../src/rubro_negra.c ../src/dataset.c"
if [ "$1" = "release" ]; then
  mkdir -p ../dist/windows ../dist/linux
  python3 -m ziglang cc -O2 -std=gnu11 -s -target x86_64-windows-gnu -o ../dist/windows/poketree.exe poketree_app.c $SRC -lws2_32 -lshell32
  python3 -m ziglang cc -O2 -std=gnu11 -s -target x86_64-linux-musl -static -o ../dist/linux/poketree poketree_app.c $SRC -lm
else
  gcc -Wall -Wextra -O2 -std=gnu11 -o ../poketree poketree_app.c $SRC -lm
fi
