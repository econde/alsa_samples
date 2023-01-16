/*
 * Programa de teste da placa de som - captura.
 * As amostras são gravadas diretamente em ficheiro.
 * https://www.alsa-project.org/wiki/ALSA_Library_API
 */
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include "config.h"

static snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

static const snd_pcm_sframes_t period_size = CONFIG_BLOCK_PERIOD;


static void progress();
static void print_samples(uint8_t *buffer, snd_pcm_sframes_t nframes);

int main (int argc, char *argv[]) {
	if (argc != 4) {
		fprintf(stderr, "usage: %s <sound_device> <file_samples> <duration>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	unsigned duration = atoi(argv[3]);

	printf("sound device: %s\n"
		"file_samples: %s\n"
		"capture period: %d\n"
		"Sample rate: %d\n"
		, argv[1], argv[2], duration, CONFIG_SAMPLE_RATE);

	snd_pcm_t *handle;
	int result = snd_pcm_open (&handle, argv[1], SND_PCM_STREAM_CAPTURE, 0);
	if (result < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n",
			 argv[1],
			 snd_strerror (result));
			 exit(EXIT_FAILURE);
	}

	result = snd_pcm_set_params(handle,
					  format,
					  SND_PCM_ACCESS_RW_INTERLEAVED,
					  CONFIG_CHANNELS,
					  CONFIG_SAMPLE_RATE,
					  1,
					  500000);   /* 0.5 sec */
	if (result < 0) {
		fprintf(stderr, "snd_pcm_set_params: %s\n",
			snd_strerror(result));
			exit(EXIT_FAILURE);
    }

	result = snd_pcm_prepare(handle);
	if (result < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
			 snd_strerror(result));
		exit(EXIT_FAILURE);
	}

	FILE *fd = fopen(argv[2], "w");
	if (fd == NULL) {
		fprintf (stderr, "Error openning file \"%s\": (%s)\n",
			argv[2], strerror(errno));
		exit(EXIT_FAILURE);
	}

	int frame_size = snd_pcm_frames_to_bytes(handle, 1);
	uint8_t buffer[period_size * frame_size];

	unsigned period_total = CONFIG_SAMPLE_RATE * duration / period_size;

	for (unsigned i = 0; i < period_total; ++i) {
		snd_pcm_sframes_t read_frames =
			snd_pcm_readi(handle, buffer, period_size);
		if (read_frames < 0) {
			fprintf (stderr, "read from audio interface failed (%s)\n",
					snd_strerror(read_frames));
			exit(EXIT_FAILURE);
		}
		size_t wrote_frames = fwrite(buffer, frame_size, read_frames, fd);
		if (wrote_frames != read_frames && ferror(fd)) {
			fprintf (stderr, "Error writing to file %s: (%s)\n", argv[2],
					strerror(errno));
			exit(EXIT_FAILURE);
		}
		progress();
//		print_samples(buffer, read_frames);
	}
	fclose(fd);
	snd_pcm_close (handle);
}

static void progress() {
	static int c = '\\';
	if (c == '\\')
		c = '|';
	else if (c == '|')
		c = '/';
	else if (c == '/')
		c = '-';
	else if (c == '-')
		c = '\\';
	putchar('\b');
	putchar(c);
	fflush(stdout);
}

static void print_samples(uint8_t *buffer, snd_pcm_sframes_t nframes) {
	int16_t *p = (int16_t *)buffer;
	for (snd_pcm_sframes_t i = 0; i < nframes; p += CONFIG_CHANNELS) {
		printf("%d ", *p);
		if ((++i % 16) == 0)
			putchar('\n');
	}
	putchar('\n');
}

