all:
	g++ -o asembler src/main.cpp src/Parser.cpp src/RegexWrapper.cpp src/FileReader.cpp src/FileWriter.cpp

clean:
	rm -rf src/Lexer.cpp
	rm -rf asembler
	rm -rf tests/projinterrupts.o tests/projmain.o 
	rm -rf tests/test_write_part1.o tests/test_write_part2.o