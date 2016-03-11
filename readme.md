 <head>
<meta charset="UTF-8">
<title>gKit2light</title>
</head> 
<link rel="stylesheet" href="github-markdown.css">
<style>
    .markdown-body {
        box-sizing: border-box;
        min-width: 200px;
        max-width: 980px;
        margin: 0 auto;
        padding: 45px;
    }
</style>
<article class="markdown-body">

# installation :

gKit2 utilise 3 librairies : sdl2 et sdl2_image, et glew, il faut les installer avant de pouvoir compiler les exemples.

les étapes sont légèrement différentes d'un système à l'autre. une fois les librairies installées, il faut générer les projets pour 
votre environnement de développement : 

- visual studio ou code blocks pour windows, 

- makefile ou code blocks pour linux, 

- makefile ou xcode pour mac os.
	

c'est l'outil premake4 qui permet de créer les projets, et les makefiles, cf section premake.

## linux

installez les paquets : libsdl2-dev, libsdl2-image-dev, libglew-dev et premake4

par exemple, pour ubuntu et ses variantes :

	sudo apt-get libsdl2-dev, libsdl2-image-dev, libglew-dev premake4


## windows

les librairies sont disponibles :

- [glew](http://glew.sourceforge.net/)

- [sdl2](https://www.libsdl.org/download-2.0.php), section development libraries

- [sdl2_image](https://www.libsdl.org/projects/SDL_image/), section development libraries
	
	
premake4 est disponible :

- [premake 4.4](http://premake.github.io/download.html), copiez le dans le répertoire des sources de gKit

le plus simple est de créer un sous répertoire, `extern` par exemple, et d'y copier les fichiers `.h`, `.dll` et `.lib`. vous devez obtenir une structure :

	gKit2light/
		premake4
		premake4.lua
		data/
			shaders/
			
		tutos/
		
		src/
			gKit/
		
		extern/
		    bin/
		    include/
			SDL2/
			    SDL.h
			    SDL_image.h
			GL/
			    glew.h
		    lib/
			glew32.dll et lib
			SDL2.dll et lib
			SDL2_image.dll et lib
			SDL2_main.lib

il faudra modifier le fichier premake4.lua avec le chemin d'accès à ce répertoire, si vous n'utilisez pas cette solution. cf section premake.


## mac os

les librairies sont disponibles :

- [sdl2](https://www.libsdl.org/download-2.0.php), section development libraries

- [sdl2_image](https://www.libsdl.org/projects/SDL_image/), section development libraries

- glew n'est pas necessaire sur mac os.


copier le contenu des fichiers .dmg dans `/Library/Frameworks`

premake4 est disponible :

- [premake 4.4](http://premake.github.io/download.html)

copiez le dans le répertoire des sources de gKit


# premake :

ouvrez un terminal, et naviguez jusqu'au répertoire contenant gKit :

- windows : cherchez `powershell` dans le menu démarrer

- linux : `ctrl`-`alt`-`t`, 

- mac os : cherchez `terminal`

_rappel :_ commandes `ls` et `cd` 

éxecuter premake4 --help, la suite devrait être assez évidente. 

- si vous voulez générer un projet visual studio, il suffit de choisir la bonne version,
	- `./premake4 vs2013`
	
- pour code blocks...
	- `./premake4 codeblocks`
	
- pour xcode...
	- `./premake4 xcode`
	
- pour des makefile (linux et mac os)...
	- `./premake4 gmake`
	

vous pouvez maintenant ouvrir la solution visual studio, le projet xcode, code blocks et compiler shader_kit ou un des tutos.

## utilisation des makefiles

les makefile peuvent générer les versions debug (cf utiliser gdb ou lldb) ou les versions release, plus rapide (2 ou 3 fois, 
interressant pour les projets avec beaucoup de calculs) :

- `make shader_kit help`, affiche la liste des projets et les options disponibles,

- `make shader_kit`, compile la version debug de shader_kit,

- `make shader_kit config=release64`, compile la version release de shader_kit,

- `make shader_kit verbose=1`, compile la version debuf de shader_kit et affiche le détail des commandes exécutées.


# premake et les projets :

pour ajouter un nouveau projet, le plus simple est de modifier premake4.lua et de regénérer le projet complet.

il y a 2 solutions :

- pour un projet avec 1 seul fichier .cpp 

- pour un projet composé de plusieurs fichiers .cpp


## projet en 1 fichier .cpp

le plus simple est l'ajouter à la liste des projets, modifiez :

	 -- description des projets		
	local projects = {
		"shader_kit"
	}

et ajoutez le votre, sans l'extension .cpp, par exemple tp1 :

	local projects = {
		"shader_kit",
		"tp1"
	}

et regénérez le projet, cf premake4 gmake / vs2013 / xcode ...

## projet avec plusieurs fichiers .cpp

ajoutez la description de votre projet :

	project("tp1")
		language "C++"
		kind "ConsoleApp"
		targetdir "bin"
		files ( gkit_files )
		files { ""tp1_main.cpp", "tp1_scene.cpp", "tp1_render.cpp" }

et regénérez le projet, cf premake4 gmake / vs2013 / xcode ...

# FAQ :

_erreur horrible dans src/gKit/window.cpp_ : 

selon la version glew, le type du dernier paramètre de la fonction window.cpp/debug change, il suffit d'ajouter un `const` 
dans la déclaration du dernier paramètre.

	ligne 230 :
	//! affiche les messages d'erreur opengl. (contexte debug core profile necessaire).
	static
	void GLAPIENTRY debug( GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, 
		void *userParam )
    
à remplacer par :

	//! affiche les messages d'erreur opengl. (contexte debug core profile necessaire).
	static
	void GLAPIENTRY debug( GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, 
		const void *userParam )	//!< const 


</article>
