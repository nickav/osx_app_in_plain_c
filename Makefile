.PHONY: all
all: build make_app

build:
	clang -framework Cocoa -framework OpenGL -o main main.c -Wno-deprecated-declarations
.PHONY: build

make_app:
	rm -rf MyApp*
	./appify -s main -n MyApp
.PHONY: make_app

