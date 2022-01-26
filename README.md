ORAN Mikail p1608112

Ceci est mon rendu pour l'UE Programmation cartes graphiques. Seulement la partie 2 du frustum culling sur carte graphique est réalisée. La cohérence est respectée et les thread du compute shader ne font pas tous d'opérations atomiques sur le compteur global.

Les fichiers sont `gpu_culling.cpp`, `gpu_culling_compute.glsl` et `gpu_culling.glsl`.

# Compilation

```
premake4 gmake
make gpu_culling
```

Lancer le programme : `bin/gpu_culling`

# Frustum Culling

Le frustum est visible dans la scène et on peut interchanger de perspective entre la caméra actuelle et celle du frustum avec la touche **P**.

Il est possible de déplacer le frustum à l'aide des touches **I J K L** ainsi que **H et N**.  Le zoom de la caméra du frustum est possible avec **U** et **O**.
