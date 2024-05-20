kaleidoscopelang: main.cpp parser.cpp parser.h ast.cpp ast.h lexer.cpp lexer.h codegen.cpp codegen.h
	clang++ -std=gnu++14 -fcolor-diagnostics -fansi-escape-codes -g `/opt/homebrew/opt/llvm/bin/llvm-config --cxxflags --ldflags --system-libs --libs core` main.cpp ast.cpp lexer.cpp parser.cpp codegen.cpp -o main
