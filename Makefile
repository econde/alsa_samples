GCC=gcc

all: playback_from_file capture_to_file

playback_from_file: playback_from_file.c
	$(GCC) -g -Wall playback_from_file.c -lasound -o playback_from_file

capture_to_file: capture_to_file.c
	$(GCC) -g -Wall capture_to_file.c -lasound -o capture_to_file

