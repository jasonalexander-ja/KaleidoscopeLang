kaleidoscopelang: main.cpp
	clang++ -std=gnu++14 -fcolor-diagnostics -fansi-escape-codes -g `/opt/homebrew/opt/llvm/bin/llvm-config --cxxflags --ldflags --system-libs --libs core` /Users/jasonalex/dev/KaleidoscopeLang/main.cpp -o /Users/jasonalex/dev/KaleidoscopeLang/main