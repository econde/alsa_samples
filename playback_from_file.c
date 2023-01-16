/*
 * Programa de teste da placa de som - reprodução.
 * As amostras são lidas diretamente de ficheiro.
 * https://www.alsa-project.org/wiki/ALSA_Library_API
 */
#include <alsa/asoundlib.h>
#include "config.h"

static snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

static const snd_pcm_sframes_t period_size = CONFIG_BLOCK_PERIOD;

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "usage: %s <sound_device> <file_samples>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	snd_pcm_t *handle;
	int result = snd_pcm_open(&handle, argv[1], SND_PCM_STREAM_PLAYBACK, 0);
	if (result < 0) {
		printf("snd_pcm_open(&handle, %s, SND_PCM_STREAM_PLAYBACK, 0): %s\n",
				argv[1], snd_strerror(result));
		exit(EXIT_FAILURE);
	}
	result = snd_pcm_set_params(handle,
					  format,
					  SND_PCM_ACCESS_RW_INTERLEAVED,
					  CONFIG_CHANNELS,
					  CONFIG_SAMPLE_RATE,
					  1,
					  500000);
	if (result < 0) {
		printf("Playback open error: %s\n", snd_strerror(result));
		exit(EXIT_FAILURE);
	}

	FILE *fd = fopen(argv[2], "r");
	if (NULL == fd) {
		fprintf(stderr, "fopen(%s, \"r\"): %s", argv[2], strerror(errno));
        exit(EXIT_FAILURE);
	}

	int frame_size = snd_pcm_frames_to_bytes(handle, 1);
	uint8_t buffer[period_size * frame_size];

	size_t read_frames = fread(buffer, frame_size, period_size, fd);

	while (read_frames > 0) {
		snd_pcm_sframes_t wrote_frames = snd_pcm_writei(handle, buffer, read_frames);
		if (wrote_frames < 0)
			wrote_frames = snd_pcm_recover(handle, wrote_frames, 0);
		if (wrote_frames < 0) {
			printf("snd_pcm_writei failed: %s\n", snd_strerror(wrote_frames));
			break;
		}
		if (wrote_frames < read_frames)
			fprintf(stderr, "Short write (expected %i, wrote %li)\n",
					read_frames, wrote_frames);

		read_frames = fread(buffer, frame_size, period_size, fd);
	}
	/* pass the remaining samples, otherwise they're dropped in close */
	result = snd_pcm_drain(handle);
	if (result < 0)
		printf("snd_pcm_drain failed: %s\n", snd_strerror(result));
	snd_pcm_close(handle);
	return 0;
}
