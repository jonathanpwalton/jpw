#include "core.h"
#include <curl/curl.h>

static CURL * curl = NULL;

int progress_callback(char const * label, curl_off_t total, curl_off_t now, curl_off_t _0, curl_off_t _1) {
	printf("%lu / %lu\n", total, now);
	return 0;
}

bool urlopen(char const * url, FILE * file, bool display) {
	if (curl == NULL) {
		require(curl = curl_easy_init());

		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, url);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, !display);

	return curl_easy_perform(curl) == CURLE_OK;
}
