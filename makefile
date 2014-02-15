
src = $(wildcard src/*.cpp)
	
trees: $(src)
	emcc -o trees.html $(src) -I src/geometry/ -s FULL_ES2=1

.PHONY : clean
clean:
	rm trees.*