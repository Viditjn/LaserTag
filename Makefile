
#LIBS = -framework OpenGL -framework GLUT
LIBS = -lGL -lglut -lm 

FLAGS = -O4 -ggdb -DMAC

all: sample_gl2 sample_gl3

clean:
	rm sample_gl2 sample_gl3

sample_gl2:
	g++ -o sample_gl2 Sample_GL2.cpp $(LIBS)

sample_gl3:
	g++ -o sample_gl3 Sample_GL3.cpp $(LIBS)
