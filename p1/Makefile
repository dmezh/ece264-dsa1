# build cli parser
  all: clean
	clang++ -fsanitize=undefined stackandqueue.cpp -o p1 -std=c++17 -Wpedantic -Wall -Wmove -Wextra -rdynamic -Wunused

  clean:
	$(RM) p1
